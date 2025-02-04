// Copyright (c) 2025 Orange. All rights reserved.
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

bool CheckDictionary(UMODLCommandLine& commandLine, const KWClass& dico, const ALString& attribTreatName,
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

void ComputeTreamentAndTargetStats(const KWTupleTableLoader& loader, UPLearningSpec& learningSpec,
				   const ALString& attribTreatName, const ALString& attribTargetName)
{
	KWTupleTable univariate;
	loader.LoadUnivariate(attribTreatName, &univariate);
	learningSpec.ComputeTreatementStats(&univariate);

	loader.LoadUnivariate(attribTargetName, &univariate);
	learningSpec.ComputeTargetStats(&univariate);
}

bool CheckCategoricalAttributeConsistency(UMODLCommandLine& commandLine, KWDataGridStats* const attribValueStats)
{
	require(attribValueStats);
	require(attribValueStats->GetAttributeNumber() > 0);

	const KWDGSAttributePartition* const attrib = attribValueStats->GetAttributeAt(0);
	if (not attrib)
	{
		commandLine.AddError("Unable to check attribute.");
		return false;
	}

	// pour permettre une analyse d'uplift, les attributs traitement et cible doivent :
	//   - etre de type categoriel
	//   - avoir au moins 2 valeurs distinctes
	bool consistent = true;
	if (attrib->GetAttributeType() != KWType::Symbol)
	{
		commandLine.AddError("Attribute should be categorical.");
		consistent = false;
	}
	if (attrib->GetInitialValueNumber() < 2)
	{
		commandLine.AddError("Attribute should have at least 2 different values.");
		consistent = false;
	}
	return consistent;
}

void AnalyseAllUsedVariables(ObjectArray& attribStats, const KWTupleTableLoader& tupleTableLoader,
			     UPLearningSpec& learningSpec, const ALString& attribTreatName,
			     const ALString& attribTargetName)
{
	require(attribStats.GetSize() == 0);
	require(tupleTableLoader.GetInputClass());
	require(tupleTableLoader.GetInputClass()->GetAttributeNumber() >= 3);
	require(not attribTreatName.IsEmpty());
	require(not attribTargetName.IsEmpty());
	require(attribTreatName != attribTargetName);

	// tupletable des variables et des attributs traitement et cible
	KWTupleTable multivariateVarUplift;

	// boucle sur les attributs pour preparer les stats avant reconstruction du dictionnaire
	StringVector svAttributeNames; // a charger dans la TupleTable
	svAttributeNames.Initialize();
	svAttributeNames.SetSize(3); // 3 attributs a charger : la variable a analyser, cible et traitement
	svAttributeNames.SetAt(1, attribTargetName); //	l'attribut cible et l'attribut traitement ne
	svAttributeNames.SetAt(2, attribTreatName);  //	changent pas entre deux analyses

	const KWClass* const kwcDico = tupleTableLoader.GetInputClass();
	for (KWAttribute* currAttrib = kwcDico->GetHeadAttribute(); currAttrib; kwcDico->GetNextAttribute(currAttrib))
	{
		// pas d'analyse sur le traitement ni sur la cible, analyse uniquement si la variable est declaree utilisee
		const ALString& attribName = currAttrib->GetName();
		if (attribName == attribTargetName or attribName == attribTreatName or not currAttrib->GetUsed())
		{
			continue;
		}

		// chargement multivarie : la table doit contenir la variable a anlayser, les valeurs de traitement et de cible
		svAttributeNames.SetAt(0, attribName);
		tupleTableLoader.LoadMultivariate(&svAttributeNames, &multivariateVarUplift);

		// calcul des stats suivant le probleme d'uplift
		UPAttributeStats* const currStats = new UPAttributeStats;
		InitAndComputeAttributeStats(*currStats, currAttrib->GetName(), currAttrib->GetType(), learningSpec,
					     multivariateVarUplift);
		attribStats.Add(currStats);
	}

	ensure(attribStats.GetSize() > 0);
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
