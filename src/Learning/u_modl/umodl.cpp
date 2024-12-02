// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl.h"
#include "KWAnalysisSpec.h"
#include "KWClassStats.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KWDatabase.h"
#include "KWSTDatabase.h"
#include "KWClassDomain.h"
#include "KWSTDatabaseTextFile.h"
#include "KWLearningSpec.h"
#include "KWDataPreparationClass.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDiscretizer.h"
#include "KWDiscretizerMODL.h"
#include "KWDiscretizerUMODL.h"
#include "KWDiscretizerUnsupervised.h"
#include "KWDRDataGrid.h"
#include "KWRecodingSpec.h"
#include "PLParallelTask.h"

constexpr auto attribTraitementName = "TRAITEMENT";
constexpr auto attribCibleName = "CIBLE";

// classe de test pour implementation de fonctions de remplacement
class KWDataPreparationClassDerived : public KWDataPreparationClass
{
public:
	void ComputeDataPreparationFromAttribStats(ObjectArray* oaAttribStats)
	{
		const ALString& sLevelMetaDataKey = KWDataPreparationAttribute::GetLevelMetaDataKey();
		// Nettoyage des specifications de preparation
		DeleteDataPreparation();

		// Duplication de la classe
		const KWClass* const classPtr = GetClass();
		kwcdDataPreparationDomain = classPtr->GetDomain()->CloneFromClass(classPtr);
		kwcDataPreparationClass = kwcdDataPreparationDomain->LookupClass(classPtr->GetName());

		// Nettoyage des meta-donnees de Level
		kwcDataPreparationClass->RemoveAllAttributesMetaDataKey(
		    KWDataPreparationAttribute::GetLevelMetaDataKey());

		// Preparation de l'attribut cible
		if (not GetTargetAttributeName().IsEmpty())
		{
			dataPreparationTargetAttribute = new KWDataPreparationTargetAttribute;
			dataPreparationTargetAttribute->InitFromAttributeValueStats(kwcDataPreparationClass,
										    GetTargetValueStats());
		}

		// Ajout d'attributs derives pour toute stats de preparation disponible (univarie, bivarie...)
		for (int i = 0; i < oaAttribStats->GetSize(); i++)
		{
			const auto preparedStats = cast(KWDataPreparationStats*, oaAttribStats->GetAt(i));

			// Meta-donne de Level sur l'attribut natif, uniquement dans le cas univarie
			if (preparedStats->GetTargetAttributeType() != KWType::None and
			    preparedStats->GetAttributeNumber() == 1)
			{
				// Recherche de l'attribute dans la classe de preparation
				KWAttribute* const nativeAttribute =
				    kwcDataPreparationClass->LookupAttribute(preparedStats->GetAttributeNameAt(0));

				// Parametrage de l'indication de Level
				nativeAttribute->GetMetaData()->SetDoubleValueAt(sLevelMetaDataKey,
										 preparedStats->GetLevel());
			}

			// Ajout d'un attribut derive s'il existe une grille de donnees
			if (preparedStats->GetPreparedDataGridStats())
			{
				// Memorisation des infos de preparation de l'attribut
				auto const dataPreparationAttribute = new KWDataPreparationAttribute;
				dataPreparationAttribute->InitFromDataPreparationStats(kwcDataPreparationClass,
										       preparedStats);
				oaDataPreparationAttributes.Add(dataPreparationAttribute);
				dataPreparationAttribute->SetIndex(oaDataPreparationAttributes.GetSize() - 1);
			}
		}
	}
};

int SearchSymbol(const SymbolVector& sv, const Symbol& s)
{
	const int size = sv.GetSize();
	for (int i = 0; i < size; i++)
	{
		if (not s.Compare(sv.GetAt(i))) // renvoie 0 si symboles egaux
		{
			return i;
		}
	}
	return size;
}

int SearchContinuous(const ContinuousVector& cv, const Continuous s)
{
	const int size = cv.GetSize();
	for (int i = 0; i < size; i++)
	{
		if (cv.GetAt(i) == s) // renvoie 0 si symboles egaux
		{
			return i;
		}
	}
	return size;
}

ALString PrepareName(const ALString& name)
{
	ALString res = name.Left(min(name.GetLength(), 30));
	res.TrimRight();
	return res;
}

KWDerivationRuleOperand* MakeSymbolOperand(int origin)
{
	auto res = new KWDerivationRuleOperand;
	res->SetType(KWType::Symbol);
	res->SetOrigin(origin);
	return res;
}

KWDerivationRuleOperand* MakeConcatOperand(const ALString& attribName)
{
	auto res = MakeSymbolOperand(KWDerivationRuleOperand::OriginAttribute);
	res->SetAttributeName(attribName);
	return res;
}

KWDerivationRuleOperand* MakeSeparatorOperand()
{
	auto res = MakeSymbolOperand(KWDerivationRuleOperand::OriginConstant);
	res->SetSymbolConstant("_");
	return res;
}

void InitConcatAttrib(KWAttribute& toInit)
{
	auto sNameTraitement = PrepareName(attribTraitementName);
	auto sNameCible = PrepareName(attribCibleName);
	toInit.SetName(sNameTraitement + "_" + sNameCible);

	// add derivation rule
	auto concatRule = new KWDRConcat;
	concatRule->DeleteAllOperands();
	concatRule->AddOperand(MakeConcatOperand(attribTraitementName));
	concatRule->AddOperand(MakeSeparatorOperand());
	concatRule->AddOperand(MakeConcatOperand(attribCibleName));
	// concatRule->SetClassName(className);

	KWDerivationRule::RegisterDerivationRule(concatRule);

	toInit.SetDerivationRule(concatRule);
	toInit.SetType(concatRule->GetType());
	// toInit.SetLoaded(false);
	toInit.SetCost(1.0);
}

KWAttribute* AddConcatenatedAttribute(KWClass& dico)
{
	auto concatAttrib = new KWAttribute;
	check(concatAttrib);
	InitConcatAttrib(*concatAttrib);
	concatAttrib->CompleteTypeInfo(&dico);
	dico.InsertAttribute(concatAttrib);
	return concatAttrib;
}

IntVector* GetDenseVectorAt(KWFrequencyTable& fTable, int idx)
{
	return cast(KWDenseFrequencyVector*, fTable.GetFrequencyVectorAt(idx))->GetFrequencyVector();
}

// // separation de tupletable en frequencytables selon les valeurs d'un attribut choisi
// void SeparateWRTSymbolAttribute(const KWTupleTable& inputTable, const int attribIdx, ObjectArray& outputArray)
// {
// 	require(attribIdx >= 0);

// 	// quelques constantes
// 	const int inputTableSize = inputTable.GetSize();
// 	const int inputTableAttribCount = inputTable.GetAttributeNumber();

// 	// suivi des valeurs de symbol vues pour l'attribut d'interet
// 	SymbolVector symbols;

// 	for (int i = 0; i < inputTableSize; i++)
// 	{
// 		const auto currentTuple = inputTable.GetAt(i);

// 		// examiner le symbol
// 		const auto& sym = currentTuple->GetSymbolAt(attribIdx);
// 		const int pos = SearchSymbol(symbols, sym);

// 		if (pos == symbols.GetSize())
// 		{
// 			// symbol pas encore vu
// 			symbols.Add(sym);

// 			// une nouvelle TupleTable doit etre creee
// 			auto newTable = new KWTupleTable;
// 			for (int att = 0; att < inputTableAttribCount; att++)
// 			{
// 				newTable->AddAttribute(inputTable.GetAttributeNameAt(att),
// 						       inputTable.GetAttributeTypeAt(att));
// 			}
// 			outputArray.Add(newTable);
// 		}

// 		// editer la TupleTable correspondante
// 		auto const outputTablePtr = cast(KWTupleTable*, outputArray.GetAt(pos));
// 		outputTablePtr->SetUpdateMode(true);
// 		auto const editTuple = outputTablePtr->GetInputTuple();
// 		for (int att = 0; att < inputTableAttribCount; att++)
// 		{
// 			switch (inputTable.GetAttributeTypeAt(att))
// 			{
// 			case KWType::Continuous:
// 				editTuple->SetContinuousAt(att, currentTuple->GetContinuousAt(att));
// 				break;

// 			case KWType::Symbol:
// 				editTuple->SetSymbolAt(att, currentTuple->GetSymbolAt(att));
// 				break;

// 			default:
// 				//TODO comment gerer ce cas ?
// 				break;
// 			}
// 		}
// 		editTuple->SetFrequency(currentTuple->GetFrequency());
// 		outputTablePtr->UpdateWithInputTuple();
// 		outputTablePtr->SetUpdateMode(false);
// 	}
// }

void RegisterDiscretizers()
{
	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODL);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerUMODL);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODLEqualFrequency);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new MHDiscretizerTruncationMODLHistogram);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualWidth);
	KWDiscretizer::RegisterDiscretizer(KWType::None, new KWDiscretizerEqualFrequency);
	KWGrouper::RegisterGrouper(KWType::Symbol, new KWGrouperMODL);
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
	auto const concatRule = initialDomain->GetClassAt(0)->GetTailAttribute()->GetDerivationRule();

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
	KWDataPreparationClassDerived dataPreparationClass;
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
	concatRule->SetClassName(trainedClassDomain->GetClassAt(0)->GetName());
	delete preparedDomain;
	dataPreparationClass.RemoveDataPreparation();
	ensure(trainedClassDomain->Check());
}

int main(int argc, char** argv)
{
	// boolean bResult;
	// KhistoCommandLine commandLine;
	// boolean bOk;
	// KWObject* kwoObject;
	// int nNumber;
	// pour detecter l'allocation a la source de la non desallocation en mode debug
	// mettre le numero de bloc non desaloue et mettre un poitn d'arret dans Standard.h ligne 686 exit(nExitCode);
	// MemSetAllocIndexExit(5642);

	//test avec paths codes en dur
	const ALString sClassFileName = "D:/Users/cedric.lecam/Downloads/data1.kdic";
	const ALString sReadFileName = "D:/Users/cedric.lecam/Downloads/data1.txt";
	const ALString sTestClassName = "data1";

	// constexpr auto attribVarName = "VAR";
	// constexpr auto attribTR_CName = "TRAITEMENT_CIBLE";

	//lecture du fichier kdic et des kwclass
	KWClassDomain* const currentDomainPtr = KWClassDomain::GetCurrentDomain();
	assert(currentDomainPtr->ReadFile(sClassFileName));
	KWClass* kwcDico = currentDomainPtr->LookupClass(sTestClassName);
	check(kwcDico);
	require(kwcDico->GetAttributeNumber() == 3);

	// ajout d'un nouvel attribut, concatenation de TRAITEMENT et CIBLE
	// la regle de derivation enregistree dans l'attribut doit etre retiree
	// de l'attribut avant la destruction de l'ensemble des regles de derivation
	// TODO ajouter la ref de l'attribut dans une liste des attributs dont la regle
	// de derivation doit etre liberee
	KWAttribute* const attribConcat = AddConcatenatedAttribute(*kwcDico);
	const ALString& attribConcatName = attribConcat->GetName();

	if (currentDomainPtr->Check())
		currentDomainPtr->Compile();

	kwcDico->Write(std::cout);

	currentDomainPtr->Write(std::cout);

	auto classptr = currentDomainPtr->GetClassAt(0);
	classptr->GetTailAttribute()->GetDerivationRule()->Write(std::cout);

	kwcDico->GetDomain()->Write(std::cout);

	std::cout << kwcDico->GetName() << '\n';
	std::cout << "nombre attribut :" << kwcDico->GetAttributeNumber() << '\n';

	KWSTDatabaseTextFile readDatabase;
	readDatabase.SetClassName(sTestClassName);
	readDatabase.SetDatabaseName(sReadFileName);
	readDatabase.SetSampleNumberPercentage(100);

	// Lecture instance par instance
	readDatabase.ReadAll();
	std::cout << "GetExactObjectNumber :" << readDatabase.GetExactObjectNumber() << '\n';

	KWDataPreparationUnivariateTask dataPreparationUnivariateTask;

	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	RegisterDiscretizers();

	// Declaration des taches paralleles
	RegisterParallelTasks();

	// Enregistrement des regles liees aux datagrids
	KWDRRegisterDataGridRules();

	const auto& varAttribName = kwcDico->GetHeadAttribute()->GetName();

	// creation du tupleLoader
	KWTupleTableLoader tupleTableLoader;
	tupleTableLoader.SetInputClass(kwcDico);
	tupleTableLoader.SetInputDatabaseObjects(readDatabase.GetObjects());

	// creation de learninspec
	KWLearningSpec learningSpec;
	learningSpec.GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("UMODL");
	learningSpec.SetClass(kwcDico);
	learningSpec.SetDatabase(&readDatabase);
	learningSpec.SetTargetAttributeName(attribConcatName);

	// calcul des stats de base
	KWTupleTable univariate;
	tupleTableLoader.LoadUnivariate(attribConcatName, &univariate);

	// recuperer les valeurs de traitement_cible trouvees
	SymbolVector symbolsSeen;
	for (int i = 0; i < univariate.GetSize(); i++)
	{
		symbolsSeen.Add(univariate.GetAt(i)->GetSymbolAt(0));
	}

	learningSpec.ComputeTargetStats(&univariate);

	// accumulation des stats d'attribut par calcul supervise selon la cible concatenee
	ObjectArray attribStats;

	// tupletable des variables et de l'attribut concatene, avec attribut concatene pour cible
	KWTupleTable bivariateVarConcat;

	// boucle sur les attributs pour preparer les stats avant reconstruction du dictionnaire
	for (KWAttribute* currAttrib = kwcDico->GetHeadAttribute(); currAttrib->GetName() != attribConcatName;
	     kwcDico->GetNextAttribute(currAttrib))
	{
		const ALString& attribName = currAttrib->GetName();

		tupleTableLoader.LoadBivariate(attribName, attribConcatName, &bivariateVarConcat);

		auto currStats = new KWAttributeStats;
		currStats->SetLearningSpec(&learningSpec);
		currStats->SetAttributeName(attribName);
		currStats->SetAttributeType(currAttrib->GetType());
		currStats->ComputeStats(&bivariateVarConcat);
		attribStats.Add(currStats);

		bivariateVarConcat.CleanAll();
	}

	// reconstruction du dictionnaire, avec stats
	KWClassDomain recodedDomain;
	BuildRecodingClass(kwcDico->GetDomain(), &attribStats, &recodedDomain);

	// TODO mettre le path dans un argument d'appel du programme
	recodedDomain.WriteFile("D:/Users/cedric.lecam/Downloads/data1_recode");

	attribStats.DeleteAll();

	// test de repartition en frequencytables suivant les valeurs de traitement
	tupleTableLoader.LoadBivariate(varAttribName, attribConcatName, &bivariateVarConcat);

	bivariateVarConcat.Write(std::cout);

	// calculer les stats par attributs. stats d'interet : ensemble des valeurs
	KWAttributeStats varStats;
	varStats.SetLearningSpec(&learningSpec);
	varStats.SetAttributeName(varAttribName);
	varStats.SetAttributeType(KWType::Continuous);
	varStats.ComputeStats(&bivariateVarConcat);
	const int nbVars = varStats.GetDescriptiveStats()->GetValueNumber();

	// alimenter la FrequencyTable avec les valeurs calculees
	KWFrequencyTable fTable;
	fTable.SetFrequencyVectorNumber(nbVars);
	//initialize FVectors
	for (int i = 0; i < fTable.GetFrequencyVectorNumber(); i++)
	{
		auto const fVec = GetDenseVectorAt(fTable, i);
		for (int j = 0; j < symbolsSeen.GetSize(); j++)
		{
			fVec->Add(0);
		}
	}

	// boucle sur les tuples
	ContinuousVector varsSeen;
	for (int i = 0; i < bivariateVarConcat.GetSize(); i++)
	{
		// var
		const auto currTuple = bivariateVarConcat.GetAt(i);
		const Continuous currVar = currTuple->GetContinuousAt(0);
		const int varIdx = SearchContinuous(varsSeen, currVar);
		if (varIdx == varsSeen.GetSize())
		{
			varsSeen.Add(currVar);
		}

		// traitement_cible
		const Symbol& currSym = currTuple->GetSymbolAt(1);
		const int symIdx = SearchSymbol(symbolsSeen, currSym);

		// maj de la FTable
		GetDenseVectorAt(fTable, varIdx)->SetAt(symIdx, currTuple->GetFrequency());
	}

	fTable.Write(std::cout);

	// separation des donnees suivant les valeurs d'un attribut de type symbol
	// auto const pivotAttrib = kwcDico->LookupAttribute(attribTraitementName);
	// ObjectArray separateTables;
	// SeparateWRTSymbolAttribute(multivariate, multivariate.LookupAttributeIndex(attribTraitementName), separateTables);

	// pour reference

	// while (attribute)
	// {
	// 	const auto& attributeName = attribute->GetName();
	// 	if (attributeName != "CIBLE")
	// 	{
	// 		// oaListAttributes.Add(attribute);
	// 		//tupleTableLoader.LoadUnivariate(attribute->GetName(), &univariateTupleTable);
	// 		tupleTableLoader.LoadBivariate(attributeName, attribute_target->GetName(),
	// 					       &bivariateTupleTable);
	// 		// Creation et initialisation d'un objet de stats pour l'attribut
	// 		KWAttributeStats* attributeStats = new KWAttributeStats;
	// 		attributeStats->SetLearningSpec(&learningSpec);
	// 		attributeStats->SetAttributeName(attributeName);
	// 		attributeStats->SetAttributeType(attribute->GetType());
	// 		// oaAttributeStats.Add(attributeStats);

	// 		// Calcul des statistitique univariee a partir de la table de tuples
	// 		attributeStats->ComputeStats(&bivariateTupleTable);
	// 		cout << "stat :" << attributeStats->GetAttributeName() << " : "
	// 		     << attributeStats->GetDescriptiveStats()->GetValueNumber() << endl;
	// 		cout << "ComputeTargetGridSize :"
	// 		     << attributeStats->GetPreparedDataGridStats()->ComputeTargetGridSize() << endl;
	// 		cout << "GetPartNumber :"
	// 		     << attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber() << endl;
	// 		bivariateTupleTable.CleanAll();
	// 	}
	// 	// Attribut suivant
	// 	kwcDico->GetNextAttribute(attribute);
	// }

	std::cout << "fin  test" << endl;

	readDatabase.GetObjects()->DeleteAll();
	// rappel : l'attribut cree par concatenation est proprietaire de sa DerivationRule.
	// comme il y a d'autres DerivationRules a liberer, celle de l'attribut doit etre dissociee
	// de celui-ci pour eviter d'acceder a de la memoire deja liberee pendant la suppression de
	// toutes les regles du meme coup
	attribConcat->RemoveDerivationRule();
	KWClassDomain::GetCurrentDomain()->DeleteAllDomains();
	// Nettoyage des taches
	PLParallelTask::DeleteAllTasks();
	// Nettoyage des methodes de pretraitement
	KWDiscretizer::DeleteAllDiscretizers();
	KWGrouper::DeleteAllGroupers();
	// nettoyage des regles de derivation
	KWDerivationRule::DeleteAllDerivationRules();
	//if (commandLine.ComputeHistogram(argc, argv);)
	return EXIT_SUCCESS;
	//else
	//	return EXIT_FAILURE;
}
