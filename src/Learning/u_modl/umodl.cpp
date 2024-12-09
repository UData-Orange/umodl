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
#include "KWDataPreparationUnivariateTask.h"
#include "KWDRDataGrid.h"
#include "PLParallelTask.h"
#include "umodlCommandLine.h"
#include "UPLearningSpec.h"
#include "UPDiscretizerUMODL.h"
#include "UPAttributeStats.h"

class Cleaner
{
public:
	// KWAttribute* m_attrib = nullptr;
	KWSTDatabaseTextFile* m_readDatabase = nullptr;
	ObjectArray* m_analysableAttributeStatsArr = nullptr;
	KWClassDomain* m_domain = nullptr;

	void operator()()
	{
		if (m_readDatabase)
		{
			m_readDatabase->GetObjects()->DeleteAll();
		}

		// if (m_attrib)
		// {
		// 	m_attrib->RemoveDerivationRule();
		// }

		if (m_analysableAttributeStatsArr)
		{
			m_analysableAttributeStatsArr->DeleteAll();
		}

		KWClassDomain::GetCurrentDomain()->DeleteAllDomains();
		// Nettoyage des taches
		PLParallelTask::DeleteAllTasks();
		// Nettoyage des methodes de pretraitement
		KWDiscretizer::DeleteAllDiscretizers();
		KWGrouper::DeleteAllGroupers();
		// nettoyage des regles de derivation
		KWDerivationRule::DeleteAllDerivationRules();
	}
};

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

void InitAndComputeAttributeStats(KWAttributeStats& stats, const ALString& name, const int type,
				  KWLearningSpec& learningSpec, const KWTupleTable& table)
{
	stats.SetLearningSpec(&learningSpec);
	stats.SetAttributeName(name);
	stats.SetAttributeType(type);
	stats.ComputeStats(&table);
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

enum class StatPreparationMode
{
	Unsupervised,
	Supervised
};

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

	Cleaner cleaner;
	UMODLCommandLine commandLine;
	UMODLCommandLine::Arguments args;
	if (not commandLine.InitializeParameters(argc, argv, args))
	{
		return EXIT_FAILURE;
	}

	const ALString& sDomainFileName = args.domainFileName;
	const ALString& sDataFileName = args.dataFileName;
	const ALString& sClassName = args.className;
	const ALString& attribTreatName = args.attribTreatName;
	const ALString& attribTargetName = args.attribTargetName;
	const ALString& outputFileName = args.outputFileName;

	//test avec paths codes en dur
	// const ALString sClassFileName = "D:/Users/cedric.lecam/Downloads/data1.kdic";
	// const ALString sReadFileName = "D:/Users/cedric.lecam/Downloads/data1.txt";
	// const ALString sTestClassName = "data1";

	// constexpr auto attribVarName = "VAR";
	// constexpr auto attribTR_CName = "TRAITEMENT_CIBLE";

	//lecture du fichier kdic et des kwclass
	KWClassDomain* const currentDomainPtr = KWClassDomain::GetCurrentDomain();
	if (not currentDomainPtr->ReadFile(sDomainFileName))
	{
		commandLine.AddError("Unable to read dictionnary file.");
		return EXIT_FAILURE;
	}

	KWClass* kwcDico = currentDomainPtr->LookupClass(sClassName);
	if (not kwcDico)
	{
		commandLine.AddError("Dictionnary does not contain class " + sClassName);
		cleaner();
		return EXIT_FAILURE;
	}

	// inspection du kdic :
	// au moins 3 attributs dont attribTreatName et attribTargetName
	// attribTreatName et attribTargetName sont categoriels
	// au moins un des autres attributs est numerique ou categoriel
	ObjectArray analysableAttribs;

	if (not CheckDictionnary(commandLine, *kwcDico, attribTreatName, attribTargetName, analysableAttribs))
	{
		commandLine.AddError("Loaded dictionnary cannot be analysed.");
		cleaner();
		return EXIT_FAILURE;
	}

	// ajout d'un nouvel attribut, concatenation de TRAITEMENT et CIBLE
	// la regle de derivation enregistree dans l'attribut doit etre retiree
	// de l'attribut avant la destruction de l'ensemble des regles de derivation
	// TODO ajouter la ref de l'attribut dans une liste des attributs dont la regle
	// de derivation doit etre liberee
	//KWAttribute* const attribConcat = AddConcatenatedAttribute(*kwcDico, attribTreatName, attribTargetName);
	//const ALString& attribConcatName = attribConcat->GetName();

	// kwcDico->InsertAttribute(attribConcat);

	if (not currentDomainPtr->Check())
	{
		commandLine.AddError("Domain is not consistent.");
		cleaner();
		return EXIT_FAILURE;
	}
	currentDomainPtr->Compile();

	// verification visuelle de quelques elements du dictionnaire
	kwcDico->Write(std::cout);
	currentDomainPtr->Write(std::cout);

	// const KWClass* const classptr = currentDomainPtr->GetClassAt(0);
	// classptr->GetTailAttribute()->GetDerivationRule()->Write(std::cout);
	kwcDico->GetDomain()->Write(std::cout);

	std::cout << kwcDico->GetName() << '\n';
	std::cout << "nombre attribut :" << kwcDico->GetAttributeNumber() << '\n';

	// lecture de la base de donnees
	KWSTDatabaseTextFile readDatabase;
	readDatabase.SetClassName(sClassName);
	readDatabase.SetDatabaseName(sDataFileName);
	readDatabase.SetSampleNumberPercentage(100);

	// Lecture instance par instance
	if (not readDatabase.ReadAll())
	{
		commandLine.AddError("Unable to read the database.");
		cleaner();
		return EXIT_FAILURE;
	}
	cleaner.m_readDatabase = &readDatabase;

	std::cout << "GetExactObjectNumber :" << readDatabase.GetExactObjectNumber() << '\n';

	// verification des donnees avant analyses

	//KWDataPreparationUnivariateTask dataPreparationUnivariateTask;

	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	RegisterDiscretizers();

	// Declaration des taches paralleles
	RegisterParallelTasks();

	// Enregistrement des regles liees aux datagrids
	KWDRRegisterDataGridRules();

	const KWAttribute* const varAttrib = kwcDico->GetHeadAttribute();
	const ALString& varAttribName = varAttrib->GetName();

	// creation du tupleLoader
	KWTupleTableLoader tupleTableLoader;
	tupleTableLoader.SetInputClass(kwcDico);
	tupleTableLoader.SetInputDatabaseObjects(readDatabase.GetObjects());

	// creation de learninspec
	UPLearningSpec learningSpec;
	learningSpec.GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("UMODL");
	learningSpec.SetClass(kwcDico);
	learningSpec.SetDatabase(&readDatabase);
	learningSpec.SetTargetAttributeName(attribTargetName);
	learningSpec.SetTreatementAttributeName(attribTreatName);

	//DDD
	cout << "Initial learning spec: " << &learningSpec << endl;

	// // calculs de stats de base en non supervise
	// // pour determiner des caracteristiques simples sur les donnees :
	// // - la cible a exactement 2 valeurs distinctes
	// // - le traitement a au moins 2 valeurs distinctes
	// // - au moins un des attributs a analyser a au moins 2 valeurs distinctes
	// KWTupleTable univariate;
	// if (not PrepareStats(attribTargetName, learningSpec, tupleTableLoader, univariate,
	// 		     StatPreparationMode::Unsupervised))
	// {
	// 	commandLine.AddError("Unable to compute stats.");
	// 	cleaner();
	// 	return EXIT_FAILURE;
	// }

	// KWAttributeStats treatStats;
	// KWAttributeStats targetStats;

	// if (not CheckTreatmentAndTarget(commandLine, treatStats, attribTreatName, targetStats, attribTargetName,
	// 				learningSpec, tupleTableLoader, univariate))
	// {
	// 	commandLine.AddError("Unable to analyse data with current treatment and target.");
	// 	cleaner();
	// 	return EXIT_FAILURE;
	// }

	// // verification des attributs potentiellement analysables : au moins 2 valeurs distinctes
	// ObjectArray analysableAttributeStatsArr;

	// if (not CheckAnalysableAttributes(commandLine, analysableAttribs, analysableAttributeStatsArr, learningSpec,
	// 				  tupleTableLoader, univariate))
	// {
	// 	commandLine.AddError("Unable to analyse data.");
	// 	cleaner();
	// 	return EXIT_FAILURE;
	// }

	// cleaner.m_analysableAttributeStatsArr = &analysableAttributeStatsArr;

	///////////////////////////////////////////////////////////////////////
	// mode supervise

	// if (not PrepareStats(attribConcatName, learningSpec, tupleTableLoader, univariate,
	// 		     StatPreparationMode::Supervised))
	// {
	// 	commandLine.AddError("Failed to compute stats on concatenated attribute.");
	// 	cleaner();
	// 	return EXIT_FAILURE;
	// }
	// tupleTableLoader.LoadUnivariate(attribConcatName, &univariate);

	KWTupleTable univariate;
	tupleTableLoader.LoadUnivariate(attribTreatName, &univariate);

	learningSpec.ComputeTreatementStats(&univariate);
	cout << "nb valeur traitement : "
	     << learningSpec.GetTreatementValueStats()->GetAttributeAt(0)->GetInitialValueNumber() << endl;

	// calcul des stats de base de la cible

	tupleTableLoader.LoadUnivariate(attribTargetName, &univariate);
	// recuperer les valeurs de traitement_cible trouvees
	SymbolVector symbolsSeen;
	for (int i = 0; i < univariate.GetSize(); i++)
	{
		symbolsSeen.Add(univariate.GetAt(i)->GetSymbolAt(0));
	}

	learningSpec.ComputeTargetStats(&univariate);

	//DDD
	cout << "Initial learning spec 2: " << &learningSpec << " " << learningSpec.GetNullPreparationCost() << endl;
	cout << "Initial learning spec 3: " << &learningSpec << " " << learningSpec.GetNullCost() << endl;

	// accumulation des stats d'attribut par calcul supervise selon la cible concatenee
	ObjectArray attribStats;

	// tupletable des variables et de l'attribut concatene, avec attribut concatene pour cible
	KWTupleTable bivariateVarConcat;
	KWTupleTable multivariatevaruplift;

	// boucle sur les attributs pour preparer les stats avant reconstruction du dictionnaire
	for (KWAttribute* currAttrib = kwcDico->GetHeadAttribute();
	     currAttrib->GetName() != attribTargetName && currAttrib->GetName() != attribTreatName;
	     kwcDico->GetNextAttribute(currAttrib))
	{
		const ALString& attribName = currAttrib->GetName();
		StringVector svAttributeNames;
		svAttributeNames.Initialize();
		//svAttributeNames.SetSize(3);
		svAttributeNames.Add(attribName);
		svAttributeNames.Add(attribTargetName);
		svAttributeNames.Add(attribTreatName);

		svAttributeNames.GetSize();
		//tupleTableLoader.LoadBivariate(attribName, attribTreatName, &bivariateVarConcat);
		tupleTableLoader.LoadMultivariate(&svAttributeNames, &multivariatevaruplift);

		UPAttributeStats* const currStats = new UPAttributeStats;
		InitAndComputeStats(*currStats, *currAttrib, learningSpec, multivariatevaruplift);
		attribStats.Add(currStats);

		//DDD
		cout << "Attribute stats learning spec: " << currStats->GetLearningSpec() << " "
		     << learningSpec.GetNullCost() << endl;

		bivariateVarConcat.CleanAll();
	}

	// reconstruction du dictionnaire, avec stats
	KWClassDomain recodedDomain;
	BuildRecodingClass(kwcDico->GetDomain(), &attribStats, &recodedDomain);

	// sauvegarde dans un fichier
	recodedDomain.WriteFile(outputFileName);

	attribStats.DeleteAll();

	// test de repartition en frequencytables suivant les valeurs de traitement
	//tupleTableLoader.LoadBivariate(varAttribName, attribConcatName, &bivariateVarConcat);

	//bivariateVarConcat.Write(std::cout);

	// calculer les stats par attributs. stats d'interet : ensemble des valeurs
	// KWAttributeStats varStats;
	// InitAndComputeStats(varStats, *varAttrib, learningSpec, multivariatevaruplift);
	// const int nbVars = varStats.GetDescriptiveStats()->GetValueNumber();

	// alimenter la FrequencyTable avec les valeurs calculees
	//KWFrequencyTable fTable;
	//fTable.SetFrequencyVectorNumber(nbVars);
	//initialize FVectors
	/* for (int i = 0; i < fTable.GetFrequencyVectorNumber(); i++)
	{
		IntVector* const fVec = GetDenseVectorAt(fTable, i);
		for (int j = 0; j < symbolsSeen.GetSize(); j++)
		{
			fVec->Add(0);
		}
	}*/

	// boucle sur les tuples
	/* ContinuousVector varsSeen;
	for (int i = 0; i < bivariateVarConcat.GetSize(); i++)
	{
		// var
		const KWTuple* const currTuple = bivariateVarConcat.GetAt(i);
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

	fTable.Write(std::cout);*/

	// separation des donnees suivant les valeurs d'un attribut de type symbol
	// auto const pivotAttrib = kwcDico->LookupAttribute(attribTreatName);
	// ObjectArray separateTables;
	// SeparateWRTSymbolAttribute(multivariate, multivariate.LookupAttributeIndex(attribTreatName), separateTables);

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

	// readDatabase.GetObjects()->DeleteAll();
	// // rappel : l'attribut cree par concatenation est proprietaire de sa DerivationRule.
	// // comme il y a d'autres DerivationRules a liberer, celle de l'attribut doit etre dissociee
	// // de celui-ci pour eviter d'acceder a de la memoire deja liberee pendant la suppression de
	// // toutes les regles du meme coup
	// attribConcat->RemoveDerivationRule();
	// KWClassDomain::GetCurrentDomain()->DeleteAllDomains();
	// // Nettoyage des taches
	// PLParallelTask::DeleteAllTasks();
	// // Nettoyage des methodes de pretraitement
	// KWDiscretizer::DeleteAllDiscretizers();
	// KWGrouper::DeleteAllGroupers();
	// // nettoyage des regles de derivation
	// KWDerivationRule::DeleteAllDerivationRules();
	//if (commandLine.ComputeHistogram(argc, argv);)
	cleaner();
	return EXIT_SUCCESS;
	//else
	//	return EXIT_FAILURE;
}
