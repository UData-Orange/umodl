// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl_utils.h"

#include "JSONFile.h"
#include "KWAnalysisSpec.h"
#include "KWDRString.h"
#include "UPAttributeStats.h"
#include "UPDataPreparationClass.h"

ALString PrepareName(const ALString& name)
{
	ALString res = name.Left(min(name.GetLength(), 30));
	res.TrimRight();
	return res;
}

KWDerivationRuleOperand* MakeSymbolOperand(int origin)
{
	KWDerivationRuleOperand* const res = new KWDerivationRuleOperand;
	check(res);
	res->SetType(KWType::Symbol);
	res->SetOrigin(origin);
	return res;
}

KWDerivationRuleOperand* MakeConcatOperand(const ALString& attribName)
{
	KWDerivationRuleOperand* const res = MakeSymbolOperand(KWDerivationRuleOperand::OriginAttribute);
	res->SetAttributeName(attribName);
	return res;
}

KWDerivationRuleOperand* MakeSeparatorOperand()
{
	KWDerivationRuleOperand* const res = MakeSymbolOperand(KWDerivationRuleOperand::OriginConstant);
	res->SetSymbolConstant("_");
	return res;
}

bool InitConcatAttrib(KWAttribute& toInit, const ALString& attribOperand1, const ALString attribOperand2)
{
	const ALString sNameOperand1 = PrepareName(attribOperand1);
	if (sNameOperand1.IsEmpty())
	{
		toInit.AddError("Trimming of attribute name " + attribOperand1 + " returned an empty string.");
		return false;
	}
	const ALString sNameOperand2 = PrepareName(attribOperand2);
	if (sNameOperand2.IsEmpty())
	{
		toInit.AddError("Trimming of attribute name " + attribOperand2 + " returned an empty string.");
		return false;
	}
	toInit.SetName(sNameOperand1 + "_" + sNameOperand2);

	// add derivation rule
	KWDRConcat* const concatRule = new KWDRConcat;
	check(concatRule);
	concatRule->DeleteAllOperands();
	concatRule->AddOperand(MakeConcatOperand(attribOperand1));
	concatRule->AddOperand(MakeSeparatorOperand());
	concatRule->AddOperand(MakeConcatOperand(attribOperand2));
	// concatRule->SetClassName(className);

	KWDerivationRule::RegisterDerivationRule(concatRule);

	toInit.SetDerivationRule(concatRule);
	toInit.SetType(concatRule->GetType());
	// toInit.SetLoaded(false);
	toInit.SetCost(1.0);

	return true;
}

KWAttribute* MakeConcatenatedAttribute(KWClass& dico, const ALString& attribOperand1, const ALString attribOperand2)
{
	require(not attribOperand1.IsEmpty() and not attribOperand2.IsEmpty());

	KWAttribute* const concatAttrib = new KWAttribute;
	if (not concatAttrib)
	{
		return nullptr;
	}

	if (not InitConcatAttrib(*concatAttrib, attribOperand1, attribOperand2))
	{
		delete concatAttrib;
		return nullptr;
	}

	concatAttrib->CompleteTypeInfo(&dico);
	return concatAttrib;
}

// adaptation de BuildRecodingClass pour un parametre d'entree de type ObjectArray
// parametre attribStats est un object array de KWAttributeStats
void BuildRecodingClass(const KWClassDomain* initialDomain, ObjectArray* const attribStats,
			KWClassDomain* const trainedClassDomain)
{
	ObjectArray oaSelectedDataPreparationAttributes;
	NumericKeyDictionary nkdSelectedDataPreparationAttributes;
	ObjectArray oaAddedAttributes;

	require(initialDomain);
	// require(initialDomain->LookupClass(GetClassName()));
	require(attribStats);
	// require(classStats->IsStatsComputed());
	// TODO verifier le statut IsStatsComputed pour chaque attribut de attribStats
	require(trainedClassDomain);

	// keep a ref to the concat rule
	// KWDerivationRule* const concatRule = initialDomain->GetClassAt(0)->GetTailAttribute()->GetDerivationRule();

	// initialiser recoding spec
	KWAnalysisSpec analysisSpec;
	KWAttributeConstructionSpec* const attribConsSpec =
	    analysisSpec.GetModelingSpec()->GetAttributeConstructionSpec();
	attribConsSpec->SetMaxConstructedAttributeNumber(1000);
	attribConsSpec->SetMaxTextFeatureNumber(10000);
	attribConsSpec->SetMaxTreeNumber(10);

	// Acces aux parametres de recodage
	const KWRecodingSpec* const recodingSpec = analysisSpec.GetRecoderSpec()->GetRecodingSpec();
	require(recodingSpec->Check());

	// Message utilisateur
	KWLearningSpec* learningSpec = cast(KWAttributeStats*, attribStats->GetAt(0))->GetLearningSpec();
	const KWClass* const kwcClass = learningSpec->GetClass();
	// AddSimpleMessage("Build data preparation dictionary " + sRecodingPrefix + kwcClass->GetName());

	// Creation de la classe de recodage
	UPDataPreparationClass dataPreparationClass;
	dataPreparationClass.SetLearningSpec(learningSpec);
	dataPreparationClass.ComputeDataPreparationFromAttribStats(attribStats);
	KWClass* const preparedClass = dataPreparationClass.GetDataPreparationClass();
	// preparedDomain = dataPreparationClass.GetDataPreparationDomain();
	KWClassDomain* const preparedDomain = dataPreparationClass.GetDataPreparationDomain();

	// Libelle de la classe
	preparedClass->SetLabel("Recoding dictionary");
	const ALString& classLabel = kwcClass->GetLabel();
	if (not classLabel.IsEmpty())
		preparedClass->SetLabel("Recoding dictionary: " + classLabel);

	// Memorisation des attributs informatifs
	if (recodingSpec->GetFilterAttributes())
	{
		const bool learningUnsupervised = learningSpec->GetTargetAttributeName().IsEmpty();
		const ObjectArray* const dataPreparationAttributesArray =
		    dataPreparationClass.GetDataPreparationAttributes();
		for (int nAttribute = 0; nAttribute < dataPreparationAttributesArray->GetSize(); nAttribute++)
		{
			KWDataPreparationAttribute* dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*, dataPreparationAttributesArray->GetAt(nAttribute));

			const KWDataPreparationStats* const preparedStats =
			    dataPreparationAttribute->GetPreparedStats();

			// S'il y a filtrage, on ne garde que ceux de valeur (Level, DeltaLevel...) strictement positive en
			// supervise,
			bool filterAttribute = preparedStats->GetSortValue() <= 0;
			// et ceux ayant au moins deux valeurs en non supervise
			if (learningUnsupervised and dataPreparationAttribute->GetNativeAttributeNumber() == 1)
			{
				filterAttribute =
				    cast(KWAttributeStats*, preparedStats)->GetDescriptiveStats()->GetValueNumber() <=
				    1;
			}

			if (not filterAttribute)
				oaSelectedDataPreparationAttributes.Add(dataPreparationAttribute);
		}
	}

	// Calcul si necessaire des attributs a traiter
	// On les memorise dans un tableau temporaire trie par valeur predictive decroissante, puis
	// dans un dictionnaire permettant de tester s'il faut les traiter
	// Cela permet ensuite de parcourir les attributs dans leur ordre initial
	const int maxFilteredAttribNumber = recodingSpec->GetMaxFilteredAttributeNumber();
	oaSelectedDataPreparationAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaSelectedDataPreparationAttributes.Sort();
	for (int nAttribute = 0; nAttribute < oaSelectedDataPreparationAttributes.GetSize(); nAttribute++)
	{
		KWDataPreparationAttribute* dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaSelectedDataPreparationAttributes.GetAt(nAttribute));

		// On selectionne l'attribut selon le nombre max demande
		if (maxFilteredAttribNumber == 0 or
		    (maxFilteredAttribNumber > 0 and
		     nkdSelectedDataPreparationAttributes.GetCount() < maxFilteredAttribNumber))
		{
			nkdSelectedDataPreparationAttributes.SetAt(dataPreparationAttribute, dataPreparationAttribute);
		}
	}
	oaSelectedDataPreparationAttributes.SetSize(0);
	assert(maxFilteredAttribNumber == 0 or
	       (maxFilteredAttribNumber > 0 and
		nkdSelectedDataPreparationAttributes.GetCount() <= maxFilteredAttribNumber));

	const int nbDataPreparationAttributes = dataPreparationClass.GetDataPreparationAttributes()->GetSize();
	// Parcours des attributs de preparation pour mettre tous les attributs natifs en Unused par defaut
	for (int nAttribute = 0; nAttribute < nbDataPreparationAttributes; nAttribute++)
	{
		KWDataPreparationAttribute* const dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

		// Parametrage des variables natives en Unused
		for (int nNative = 0; nNative < dataPreparationAttribute->GetNativeAttributeNumber(); nNative++)
			dataPreparationAttribute->GetNativeAttributeAt(nNative)->SetUsed(false);
	}

	// Parcours des attributs de preparation, dans le meme ordre que celui des attributs prepares
	const bool recodeProbabilisticDistance = recodingSpec->GetRecodeProbabilisticDistance();
	const ALString& recodeContinuousAttributes = recodingSpec->GetRecodeContinuousAttributes();
	const ALString& recodeSymbolAttributes = recodingSpec->GetRecodeSymbolAttributes();
	const ALString& recodeBivariateAttributes = recodingSpec->GetRecodeBivariateAttributes();

	for (int nAttribute = 0; nAttribute < nbDataPreparationAttributes; nAttribute++)
	{
		KWDataPreparationAttribute* dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

		const int nbNativeAttributes = dataPreparationAttribute->GetNativeAttributeNumber();

		// Filtrage de l'attribut s'il n'est pas informatif
		const bool bFilterAttribute = not nkdSelectedDataPreparationAttributes.Lookup(dataPreparationAttribute);

		// Creation des variables recodees
		dataPreparationAttribute->GetPreparedAttribute()->SetUsed(false);
		if (not bFilterAttribute)
		{
			// Recodage selon la distance probabiliste (mode expert uniquement)
			// Chaque variable servant a mesurer la distance entre deux individus est suivi
			// d'une variable d'auto-distance, a utiliser uniquement en cas d'egalite de distance
			if (recodeProbabilisticDistance)
			{
				assert(GetDistanceStudyMode());
				dataPreparationAttribute->AddPreparedDistanceStudyAttributes(&oaAddedAttributes);
			}
			else
			{
				//univarie ou multivarie
				assert(nbNativeAttributes >= 1);
				const bool isUnivariate = nbNativeAttributes == 1;

				const int nativeAttribType = dataPreparationAttribute->GetNativeAttribute()->GetType();
				const ALString& recodeAttributes =
				    isUnivariate
					? ((nativeAttribType == KWType::Continuous) ? recodeContinuousAttributes
										    : recodeSymbolAttributes)
					: recodeBivariateAttributes;

				// Recodage par identifiant de partie
				if (recodeAttributes == "part Id")
					dataPreparationAttribute->AddPreparedIdAttribute();
				// Recodage par libelle de partie
				else if (recodeAttributes == "part label")
					dataPreparationAttribute->AddPreparedLabelAttribute();
				// Recodage disjonctif complet de l'identifiant de partie
				else if (recodeAttributes == "0-1 binarization")
					dataPreparationAttribute->AddPreparedBinarizationAttributes(&oaAddedAttributes);
				// Recodage par les informations conditionnelles de la source sachant la cible
				else if (recodeAttributes == "conditional info")
					dataPreparationAttribute->AddPreparedSourceConditionalInfoAttributes(
					    &oaAddedAttributes);
				// traitement particulier pour univarie Continuous
				else if (isUnivariate and nativeAttribType == KWType::Continuous)
				{
					// Normalisation par centrage-reduction
					if (recodeAttributes == "center-reduction")
						dataPreparationAttribute->AddPreparedCenterReducedAttribute();
					// Normalisation 0-1
					else if (recodeAttributes == "0-1 normalization")
						dataPreparationAttribute->AddPreparedNormalizedAttribute();
					// Normalisation par le rang
					else if (recodeAttributes == "rank normalization")
						dataPreparationAttribute->AddPreparedRankNormalizedAttribute();
				}
			}
		}

		// Transfer des variables natives, si elle doivent etre utilise au moins une fois
		// (une variable native peut intervenir dans plusieurs attributs prepares (e.g: bivariate))
		for (int nNative = 0; nNative < nbNativeAttributes; nNative++)
		{
			KWAttribute* const prepNativeAttribute =
			    dataPreparationAttribute->GetNativeAttributeAt(nNative);
			const int prepNativeAttribType = prepNativeAttribute->GetType();
			bool keepInitial = true;
			if (prepNativeAttribType == KWType::Continuous)
			{
				keepInitial = recodingSpec->GetKeepInitialContinuousAttributes();
			}
			else if (prepNativeAttribType == KWType::Symbol)
			{
				keepInitial = recodingSpec->GetKeepInitialSymbolAttributes();
			}
			const bool newUsed = prepNativeAttribute->GetUsed() or (not bFilterAttribute and keepInitial);

			prepNativeAttribute->SetUsed(newUsed);
			prepNativeAttribute->SetLoaded(newUsed);
		}
	}

	// On passe tous les attributs non simple en Unused
	for (KWAttribute* attribute = preparedClass->GetHeadAttribute(); attribute;
	     preparedClass->GetNextAttribute(attribute))
	{
		if (not KWType::IsSimple(attribute->GetType()))
			attribute->SetUsed(false);
	}

	// Supression des attribut inutiles (necessite une classe compilee)
	// KWClassDomain* const preparedDomain = dataPreparationClass.GetDataPreparationDomain();
	preparedDomain->Compile();
	preparedClass->DeleteUnusedDerivedAttributes(initialDomain);

	// Transfert du domaine de preparation dans le domaine cible
	trainedClassDomain->ImportDomain(preparedDomain, "R_", "");
	// maj des regles de derivation
	// concatRule->SetClassName(trainedClassDomain->GetClassAt(0)->GetName());
	delete preparedDomain;
	dataPreparationClass.RemoveDataPreparation();
	ensure(trainedClassDomain->Check());
}

void InitAndComputeStats(KWAttributeStats& attribStats, const KWAttribute& attrib, KWLearningSpec& learningSpec,
			 const KWTupleTable& tupleTable)
{
	attribStats.SetLearningSpec(&learningSpec);
	attribStats.SetAttributeName(attrib.GetName());
	attribStats.SetAttributeType(attrib.GetType());
	attribStats.ComputeStats(&tupleTable);
}

void WriteDictionnary(JSONFile& file, const ALString& key, const ObjectArray& attribsUpliftStats, const bool summary)
{
	file.BeginKeyArray(key);
	for (int i = 0; i < attribsUpliftStats.GetSize(); i++)
	{
		UPAttributeStats* currAttribStats = cast(UPAttributeStats*, attribsUpliftStats.GetAt(i));
		file.BeginKeyObject(currAttribStats->GetIdentifier());
		currAttribStats->WriteJSONArrayFields(&file, summary);
		file.EndObject();
	}
	file.EndArray();
}

void WriteJSONReport(const UPLearningSpec& learningSpec, ObjectArray& attribStats)
{
	// ouvre un fichier JSON
	JSONFile fJSON;

	ALString sJSONReportName = "./reportfilepath.json";
	fJSON.SetFileName(sJSONReportName);
	fJSON.OpenForWrite();

	if (fJSON.IsOpened())
	{
		// Outil et version
		fJSON.WriteKeyString("tool", "UMODL");
		fJSON.WriteKeyString("version", "V0");

		// rapport de preparation minimaliste : seulement les specifications d'apprentissage

		learningSpec.GetTargetDescriptiveStats()->WriteJSONKeyReport(&fJSON, "targetDescriptiveStats");
		learningSpec.GetTargetValueStats()->WriteJSONKeyValueFrequencies(&fJSON, "targetValues");
		learningSpec.GetTreatementDescriptiveStats()->WriteJSONKeyReport(&fJSON, "treatmentDescriptiveStats");
		learningSpec.GetTreatementValueStats()->WriteJSONKeyValueFrequencies(&fJSON, "treatmentValues");

		const int attribSize = attribStats.GetSize();

		// nombre de variables evaluees
		fJSON.WriteKeyInt("evaluatedVariables", attribSize);

		// algorithmes utilises
		fJSON.WriteKeyString("discretization", "UMODL");

		// calcul des identifiants bases sur les rangs
		UPAttributeStats* instance = cast(UPAttributeStats*, attribStats.GetAt(0));
		instance->ComputeRankIdentifiers(&attribStats);

		// rapports synthetiques
		WriteDictionnary(fJSON, "attributes", attribStats, true);

		// rapports detailles
		WriteDictionnary(fJSON, "detailed statistics", attribStats, false);
	}

	fJSON.Close();
}
