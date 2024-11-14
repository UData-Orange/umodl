// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodl.h"
#include "KWDatabase.h"
#include "KWSTDatabase.h"
#include "KWClassDomain.h"
#include "KWSTDatabaseTextFile.h"
#include "KWLearningSpec.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWDiscretizer.h"
#include "KWDiscretizerMODL.h"
#include "KWDiscretizerUnsupervised.h"

int main(int argc, char** argv)
{
	boolean bResult;
	KhistoCommandLine commandLine;
	boolean bOk;
	KWObject* kwoObject;
	int nNumber;
	KWLearningSpec learningSpec;
	// pour detecter l'allocation a la source de la non desallocation en mode debug
	// mettre le numero de bloc non desaloue et mettre un poitn d'arret dans Standard.h ligne 686 exit(nExitCode);
	//MemSetAllocIndexExit(4346);

	//test nicolas
	KWClass* kwcDico;
	ALString sClassFileName = "C:/DEV/GIT/umodl/build/windows-msvc-release/bin/data1.kdic";
	ALString sClassName;
	ALString sReadFileName = "C:/DEV/GIT/umodl/build/windows-msvc-release/bin/data1.txt";
	ALString sTestClassName = "data1";

	KWSTDatabaseTextFile readDatabase;
	//KWClassDomain dico;

	//lecture du fichier kdic et des kwclass
	KWClassDomain::GetCurrentDomain()->ReadFile(sClassFileName);
	kwcDico = KWClassDomain::GetCurrentDomain()->LookupClass(sTestClassName);
	if (KWClassDomain::GetCurrentDomain()->Check())
		KWClassDomain::GetCurrentDomain()->Compile();

	cout << kwcDico->GetName() << endl;
	cout << "nombre attribut :" << kwcDico->GetAttributeNumber() << endl;

	readDatabase.SetClassName(sTestClassName);
	readDatabase.SetDatabaseName(sReadFileName);
	readDatabase.SetSampleNumberPercentage(100);

	// Lecture instance par instance
	readDatabase.ReadAll();
	cout << "GetExactObjectNumber :" << readDatabase.GetExactObjectNumber() << endl;

	//creation de learninspec
	learningSpec.SetClass(kwcDico);
	learningSpec.SetDatabase(&readDatabase);
	learningSpec.SetTargetAttributeName("CIBLE");

	// creation du tupleLoader
	KWTupleTableLoader tupleTableLoader;
	tupleTableLoader.SetInputClass(kwcDico);
	tupleTableLoader.SetInputDatabaseObjects(readDatabase.GetObjects());

	// calcul de la preparation des variables

	// Enregistrement des methodes de pretraitement supervisees et non supervisees
	KWDiscretizer::RegisterDiscretizer(KWType::Symbol, new KWDiscretizerMODL);
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

	KWDataPreparationUnivariateTask dataPreparationUnivariateTask;
	ObjectArray oaAttributeStats;
	ObjectArray oaListAttributes;
	KWAttribute* attribute;
	KWAttribute* attribute_target;
	KWAttributeStats* attributeStats;
	KWTupleTable univariateTupleTable;
	attribute_target = kwcDico->LookupAttribute("CIBLE");
	tupleTableLoader.LoadUnivariate(attribute_target->GetName(), &univariateTupleTable);
	learningSpec.ComputeTargetStats(&univariateTupleTable);
	univariateTupleTable.CleanAll();

	attribute = kwcDico->GetHeadAttribute();

	while (attribute != NULL)
	{
		if (attribute->GetName() != "CIBLE")
		{
			oaListAttributes.Add(attribute);
			//tupleTableLoader.LoadUnivariate(attribute->GetName(), &univariateTupleTable);
			tupleTableLoader.LoadBivariate(attribute->GetName(), attribute_target->GetName(),
						       &univariateTupleTable);
			// Creation et initialisation d'un objet de stats pour l'attribut
			attributeStats = new KWAttributeStats;
			attributeStats->SetLearningSpec(&learningSpec);
			attributeStats->SetAttributeName(attribute->GetName());
			attributeStats->SetAttributeType(attribute->GetType());
			oaAttributeStats.Add(attributeStats);

			// Calcul des statistitique univariee a partir de la table de tuples
			bOk = attributeStats->ComputeStats(&univariateTupleTable);
			cout << "stat :" << attributeStats->GetAttributeName() << " : "
			     << attributeStats->GetDescriptiveStats()->GetValueNumber() << endl;
			cout << "ComputeTargetGridSize :"
			     << attributeStats->GetPreparedDataGridStats()->ComputeTargetGridSize() << endl;
			cout << "GetPartNumber :"
			     << attributeStats->GetPreparedDataGridStats()->GetAttributeAt(0)->GetPartNumber() << endl;
			univariateTupleTable.CleanAll();
		}
		// Attribut suivant
		kwcDico->GetNextAttribute(attribute);
	}

	cout << "fin  test" << endl;
	bResult = true;

	readDatabase.GetObjects()->DeleteAll();
	KWClassDomain::GetCurrentDomain()->DeleteAllDomains();
	oaAttributeStats.DeleteAll();
	// Nettoyage des methodes de pretraitement
	KWDiscretizer::DeleteAllDiscretizers();
	KWGrouper::DeleteAllGroupers();
	//bResult = commandLine.ComputeHistogram(argc, argv);
	if (bResult)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
