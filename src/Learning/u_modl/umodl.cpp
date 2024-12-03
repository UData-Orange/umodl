// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl.h"
#include "umodl_utils.h"
#include "KWClassStats.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "KWDatabase.h"
#include "KWDiscretizer.h"
#include "KWDiscretizerUnsupervised.h"
#include "KWSTDatabase.h"
#include "KWClassDomain.h"
#include "KWSTDatabaseTextFile.h"
#include "KWLearningSpec.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDRDataGrid.h"
#include "PLParallelTask.h"
#include "UPDiscretizerUMODL.h"

constexpr auto attribTraitementName = "TRAITEMENT";
constexpr auto attribCibleName = "CIBLE";

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
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new UPDiscretizerUMODL);
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
	KWAttribute* const attribConcat = AddConcatenatedAttribute(*kwcDico, attribTraitementName, attribCibleName);
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
