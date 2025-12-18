// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl.h"
#include "umodl_utils.h"
#include "umodlCommandLine.h"

#include "KWAttribute.h"
#include "KWSTDatabaseTextFile.h"
#include "KWClassDomain.h"
#include "KWDRDataGrid.h"
#include "KWTupleTableLoader.h"
#include "Object.h"
#include "Standard.h"
#include "UPLearningSpec.h"
#include "UPAttributeStats.h"
#include "Vector.h"

int main(int argc, char** argv)
{
	// pour detecter l'allocation a la source de la non desallocation en mode debug
	// mettre le numero de bloc non desaloue et mettre un point d'arret dans Standard.h ligne 686 exit(nExitCode);
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
	const ALString& reportJSONFileName = args.reportJSONFileName;
	const int nMaxPartNumber = args.maxPartNumber;

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

	if (not CheckDictionary(commandLine, *kwcDico, attribTreatName, attribTargetName, analysableAttribs))
	{
		commandLine.AddError("Loaded dictionnary cannot be analysed.");
		cleaner();
		return EXIT_FAILURE;
	}

	if (not currentDomainPtr->Check())
	{
		commandLine.AddError("Domain is not consistent.");
		cleaner();
		return EXIT_FAILURE;
	}
	currentDomainPtr->Compile();

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

	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	RegisterDiscretizers();

	// Declaration des taches paralleles
	RegisterParallelTasks();

	// Enregistrement des regles liees aux datagrids
	KWDRRegisterDataGridRules();

	// creation du tupleLoader
	KWTupleTableLoader tupleTableLoader;
	tupleTableLoader.SetInputClass(kwcDico);
	tupleTableLoader.SetInputDatabaseObjects(readDatabase.GetObjects());

	// creation de learninspec
	UPLearningSpec learningSpec;
	learningSpec.GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName("UMODL");
	learningSpec.GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName("UMODL");
	learningSpec.SetClass(kwcDico);
	learningSpec.SetDatabase(&readDatabase);
	learningSpec.SetTargetAttributeName(attribTargetName);
	learningSpec.SetTreatementAttributeName(attribTreatName);
	learningSpec.GetPreprocessingSpec()->GetDiscretizerSpec()->SetMaxIntervalNumber(nMaxPartNumber);
	learningSpec.GetPreprocessingSpec()->GetGrouperSpec()->SetMaxGroupNumber(nMaxPartNumber);

	///////////////////////////////////////////////////////////////////////
	// mode supervise

	// calcul des stats de base de la cible
	ComputeTreamentAndTargetStats(tupleTableLoader, learningSpec, attribTreatName, attribTargetName);

	// verification des valeurs de traitement
	if (not CheckCategoricalAttributeConsistency(commandLine, learningSpec.GetTreatementValueStats()))
	{
		cout << "Treatment attribute is not consistent for uplift analysis.\n";
		cleaner();
		return EXIT_FAILURE;
	}

	// verification des valeurs de cible
	if (not CheckCategoricalAttributeConsistency(commandLine, learningSpec.GetTargetValueStats()))
	{
		cout << "Target attribute is not consistent for uplift analysis.\n";
		cleaner();
		return EXIT_FAILURE;
	}

	// accumulation des stats d'attribut par calcul supervise selon le traitement et la cible
	ObjectArray attribStats;
	AnalyseAllUsedVariables(attribStats, tupleTableLoader, learningSpec, attribTreatName, attribTargetName);

	// edition du rapport sur les stats des variables
	WriteJSONReport(reportJSONFileName, learningSpec, attribStats);

	// reconstruction du dictionnaire, avec stats
	KWClassDomain recodedDomain;
	BuildRecodingClass(kwcDico->GetDomain(), &attribStats, &recodedDomain);

	// sauvegarde dans un fichier
	recodedDomain.WriteFile(outputFileName);

	// nettoyage des UPAttributeStats crees par AnalyseAllUsedVariables
	attribStats.DeleteAll();

	cleaner();
	return EXIT_SUCCESS;
}
