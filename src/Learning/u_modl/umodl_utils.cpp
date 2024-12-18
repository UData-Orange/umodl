// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl_utils.h"

#include "JSONFile.h"
#include "KWAnalysisSpec.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDiscretizer.h"
#include "KWDiscretizerUnsupervised.h"
#include "KWDRString.h"
#include "MHDiscretizerTruncationMODLHistogram.h"
#include "UPAttributeStats.h"
#include "UPDataPreparationClass.h"
#include "UPDiscretizerUMODL.h"
#include "UPGrouperUMODL.h"

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

void InitAndComputeAttributeStats(KWAttributeStats& stats, const ALString& name, const int type,
				  KWLearningSpec& learningSpec, const KWTupleTable& table)
{
	stats.SetLearningSpec(&learningSpec);
	stats.SetAttributeName(name);
	stats.SetAttributeType(type);
	stats.ComputeStats(&table);
}

// void InitAndComputeStats(KWAttributeStats& attribStats, const KWAttribute& attrib, KWLearningSpec& learningSpec,
// 			 const KWTupleTable& tupleTable)
// {
// 	attribStats.SetLearningSpec(&learningSpec);
// 	attribStats.SetAttributeName(attrib.GetName());
// 	attribStats.SetAttributeType(attrib.GetType());
// 	attribStats.ComputeStats(&tupleTable);
// }

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

bool CheckDictionnary(UMODLCommandLine& commandLine, const KWClass& dico, const ALString& attribTreatName,
		      const ALString& attribTargetName, ObjectArray& analysableAttribs)
{
	require(analysableAttribs.GetSize() == 0);
	require(not attribTreatName.IsEmpty());
	require(not attribTargetName.IsEmpty());

	// au moins 3 attributs
	if (dico.GetAttributeNumber() < 3)
	{
		commandLine.AddError("Dictionnary contains less than 3 attributes.");
		return false;
	}

	// attribTreatName et attribTargetName doivent faire partie des attributs
	// attribTreatName et attribTargetName doivent etre categoriels
	// au moins un des autres attributs est numerique ou categoriel

	bool hasTreat = false;
	bool hasTarget = false;

	for (KWAttribute* currAttrib = dico.GetHeadAttribute(); currAttrib; dico.GetNextAttribute(currAttrib))
	{
		const ALString& name = currAttrib->GetName();
		const int currType = currAttrib->GetType();

		const bool isSymbol = currType == KWType::Symbol;

		if (not hasTreat or not hasTarget and isSymbol)
		{
			if (name == attribTreatName)
			{
				hasTreat = true;
			}
			else if (name == attribTargetName)
			{
				hasTarget = true;
			}
			else
			{
				analysableAttribs.Add(currAttrib);
			}
		}
		else if (isSymbol or currType == KWType::Continuous)
		{
			analysableAttribs.Add(currAttrib);
		}
	}

	bool res = true;

	if (analysableAttribs.GetSize() == 0)
	{
		commandLine.AddError("Dictionnary does not contain an attribute to be analyzed.");
		res = false;
	}

	if (not hasTreat)
	{
		commandLine.AddError("Dictionnary does not contain treatment attribute: " + attribTreatName);
		res = false;
	}

	if (not hasTarget)
	{
		commandLine.AddError("Dictionnary does not contain target attribute: " + attribTargetName);
		res = false;
	}

	return res;
}

bool CheckTreatmentAndTarget(UMODLCommandLine& commandLine, KWAttributeStats& treatStats,
			     const ALString& attribTreatName, KWAttributeStats& targetStats,
			     const ALString& attribTargetName, KWLearningSpec& learningSpec, KWTupleTableLoader& loader,
			     KWTupleTable& univariate)
{
	loader.LoadUnivariate(attribTargetName, &univariate);
	InitAndComputeAttributeStats(targetStats, attribTargetName, KWType::Symbol, learningSpec, univariate);

	if (GetValueNumber(targetStats) != 2)
	{
		commandLine.AddError("Target attribute does not have 2 distinct values, unable to analyse.");
		return false;
	}

	loader.LoadUnivariate(attribTreatName, &univariate);
	InitAndComputeAttributeStats(treatStats, attribTreatName, KWType::Symbol, learningSpec, univariate);

	if (GetValueNumber(treatStats) < 2)
	{
		commandLine.AddError(
		    "Treatment attribute does not have at least 2 two distinct values, unable to analyse.");
		return false;
	}

	return true;
}

bool CheckAnalysableAttributes(UMODLCommandLine& commandLine, const ObjectArray& analysableAttribsInput,
			       ObjectArray& analysableAttributeStatsArrOutput, KWLearningSpec& learningSpec,
			       KWTupleTableLoader& loader, KWTupleTable& univariate)
{
	require(analysableAttribsInput.GetSize() > 0);
	require(analysableAttributeStatsArrOutput.GetSize() == 0);

	IntVector toSuppress;
	for (int i = 0; i < analysableAttribsInput.GetSize(); i++)
	{
		const KWAttribute* const currAttrib = cast(KWAttribute*, analysableAttribsInput.GetAt(i));
		KWAttributeStats* const attribStats = new KWAttributeStats;
		if (not attribStats)
		{
			return false;
		}

		loader.LoadUnivariate(currAttrib->GetName(), &univariate);
		InitAndComputeAttributeStats(*attribStats, currAttrib->GetName(), currAttrib->GetType(), learningSpec,
					     univariate);

		if (GetValueNumber(*attribStats) > 1)
		{
			analysableAttributeStatsArrOutput.Add(attribStats);
		}
		else
		{
			toSuppress.Add(i);
			delete attribStats;
		}
	}
	for (int i = 0; i < toSuppress.GetSize(); i++)
	{
		analysableAttributeStatsArrOutput.RemoveAt(toSuppress.GetAt(i));
	}

	// verifier qu'il y a bien au moins un attribut analysable
	if (analysableAttributeStatsArrOutput.GetSize() == 0)
	{
		commandLine.AddError("No attribute to analyse.");
		return false;
	}

	return true;
}

bool PrepareStats(const ALString& attributeName, KWLearningSpec& learningSpec, KWTupleTableLoader& loader,
		  KWTupleTable& univariate, const StatPreparationMode mode)
{
	if (mode == StatPreparationMode::Supervised)
	{
		learningSpec.SetTargetAttributeName(attributeName);
	}
	else
	{
		learningSpec.SetTargetAttributeName("");
	}
	loader.LoadUnivariate(attributeName, &univariate);
	return learningSpec.ComputeTargetStats(&univariate);
}

void WriteJSONReport(const ALString& sJSONReportName, const UPLearningSpec& learningSpec, ObjectArray& attribStats)
{
	// ouvre un fichier JSON
	JSONFile fJSON;

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
		instance->WriteJSONArrayReport(&fJSON, "attributes", &attribStats, true);

		// rapports detailles
		instance->WriteJSONArrayReport(&fJSON, "detailed statistics", &attribStats, false);
	}

	fJSON.Close();
}

void RegisterDiscretizers()
{
	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODL);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new UPDiscretizerUMODL);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new MHDiscretizerTruncationMODLHistogram);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualFrequency);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperMODL);
	KWGrouper::RegisterGrouper(KWType::Symbol, new UPGrouperUMODL);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperBasicGrouping);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperMODLBasic);
	KWGrouper::RegisterGrouper(KWType::None, new KWGrouperBasicGrouping);
	KWGrouper::RegisterGrouper(KWType::None, new KWGrouperUnsupervisedMODL);
}

void RegisterParallelTasks()
{
	// Declaration des taches paralleles
	PLParallelTask::RegisterTask(new KWFileIndexerTask);
	// PLParallelTask::RegisterTask(new KWFileKeyExtractorTask);
	// PLParallelTask::RegisterTask(new KWChunkSorterTask);
	// PLParallelTask::RegisterTask(new KWKeySampleExtractorTask);
	// PLParallelTask::RegisterTask(new KWSortedChunkBuilderTask);
	PLParallelTask::RegisterTask(new KWKeySizeEvaluatorTask);
	PLParallelTask::RegisterTask(new KWKeyPositionSampleExtractorTask);
	PLParallelTask::RegisterTask(new KWKeyPositionFinderTask);
	// PLParallelTask::RegisterTask(new KWDatabaseCheckTask);
	// PLParallelTask::RegisterTask(new KWDatabaseTransferTask);
	// PLParallelTask::RegisterTask(new KWDatabaseBasicStatsTask);
	PLParallelTask::RegisterTask(new KWDatabaseSlicerTask);
	PLParallelTask::RegisterTask(new KWDataPreparationUnivariateTask);
	// PLParallelTask::RegisterTask(new KWDataPreparationBivariateTask);
	// PLParallelTask::RegisterTask(new KWClassifierEvaluationTask);
	// PLParallelTask::RegisterTask(new KWRegressorEvaluationTask);
	// PLParallelTask::RegisterTask(new KWClassifierUnivariateEvaluationTask);
	// PLParallelTask::RegisterTask(new KWRegressorUnivariateEvaluationTask);
	// PLParallelTask::RegisterTask(new SNBPredictorSelectiveNaiveBayesTrainingTask);
	// PLParallelTask::RegisterTask(new KDSelectionOperandSamplingTask);
	// PLParallelTask::RegisterTask(new DTDecisionTreeCreationTask);
	// PLParallelTask::RegisterTask(new KDTextTokenSampleCollectionTask);
}
