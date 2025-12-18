// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "umodlCommandLine.h"
#include <cstdlib>

const ALString UMODLCommandLine::GetClassLabel() const
{
	return "umodl";
}

boolean UMODLCommandLine::InitializeParameters(int argc, char** argv, Arguments& res)
{
	// On recherche deja les options standard
	ALString sArgument;
	for (int i = 1; i < argc; i++)
	{
		sArgument = argv[i];

		// Version
		if (sArgument == "-v")
		{
			std::cout << GetClassLabel() << " " << UMODL_VERSION << "\n "
				  << "Copyright (C) 2025 Orange labs\n";
			return false;
		}
		// Aide
		else if (sArgument == "-h" or sArgument == "--h" or sArgument == "--help")
		{
			ShowHelp();
			return false;
		}
	}

	// Test du bon nombre d'options
	if (argc < 6 || argc > 7)
	{
		const ALString& classLabel = GetClassLabel();
		ALString errMsg =
		    classLabel + ": invalid number of parameters\nTry '" + classLabel + "' -h' for more information.\n";
		AddError(errMsg);
		return false;
	}

	// On recopie le parametrage
	res.dataFileName = argv[1];
	res.domainFileName = argv[2];
	res.className = argv[3];
	res.attribTreatName = argv[4];
	res.attribTargetName = argv[5];
	res.maxPartNumber = 2;  // Valeur par defaut si argument non specifie
	if (argc >= 7)  // Traitement du parametre MAXPARTNUMBER
	{
		res.maxPartNumber = atoi((const char *)argv[6]);
		if (res.maxPartNumber == 0)
		{
			std::cout << "MAXPARTNUMBER is not a valid number.\n";
			return false;
		}
		if (res.maxPartNumber < 2)
		{
			std::cout << "MAXPARTNUMBER must be greater than or equal to 2.\n";
			return false;
		}
	}

	if (res.dataFileName == res.domainFileName)
	{
		std::cout << "The two file names must be different.\n";
		return false;
	}

	if (res.attribTreatName == res.attribTargetName)
	{
		std::cout << "Treatment and Target variables must be different.\n";
		return false;
	}

	// verification de l'extension du fichier dictionnaire
	const int extPos = res.domainFileName.ReverseFind('.');
	if (extPos <= 0)
	{
		AddError("Argument for dictionary filename has no extension.");
		return false;
	}
	const ALString fileExt = res.domainFileName.Right(res.domainFileName.GetLength() - extPos);
	if (fileExt != ".kdic")
	{
		AddError("Extension in argument for dictionary filename is not consistent, i.e. not .kdic.");
		return false;
	}

	// preparation des noms de fichier pour le dictionnaire recode et le rapport json
	// leur nom suit le schema <path jusqu'au dernier separateur>UP_<nom du fichier dictionnaire><extension>

	// trouver le dernier separateur dans le nom de fichier, s'il y en a un
	// separateur type windows ?
	int separatorPos = res.domainFileName.ReverseFind('\\');
	if (separatorPos < 0)
	{
		// separateur type POSIX ?
		separatorPos = res.domainFileName.ReverseFind('/');
	}
	if (separatorPos < 0)
	{
		// path relatif sans separateur
		separatorPos = -1;
	}

	// path jusqu'au dernier repertoire
	const ALString filePath = res.domainFileName.Left(separatorPos + 1);
	// nom du fichier sans le path ni l'extension
	const ALString fileShortName = res.domainFileName.Mid(separatorPos + 1, extPos - (separatorPos + 1));
	// prefix des nouveaux fichiers de resultat
	const ALString filePrefix = filePath + "UP_" + fileShortName;
	// fichiers de resultat
	res.outputFileName = filePrefix + ".kdic";
	res.reportJSONFileName = filePrefix + ".json";

	return true;
}

void UMODLCommandLine::ShowHelp()
{
	cout << "Usage: " << GetClassLabel() << " [DATAFILENAME] [DICTIONARY.kdic] [CLASS] [TREATMENT] [TARGET] [[MAXPARTNUMBER]]\n"
	     << "Compute uplift statistics from the data in DATAFILENAME.\n"
	     << "DICTIONARY.kdic describes the names and types of the variables of the associated data in "
		"DATAFILENAME.\n"
	     << "CLASS declares the name of the class in DICTIONARY.kdic corresponding to the data in DATAFILENAME.\n"
	     << "TREATMENT and TARGET declare which variables in DICTIONARY.kdic are used as the uplift treatment "
		"variable\n"
	     << "and the target variable for the uplift analysis.\n"
	     << "MAXPARTNUMBER is optional and sets the maximum number of intervals or groups. Defaults to 2.\n"
	     << "A recoded dictionary is output in UP_DICTIONARY.kdic.\n"
	     << "A report of the statistics of the variables is output as a JSON file in UP_DICTIONARY.json.\n";

	// Options generales
	cout << '\n'
	     << "\t-h\tdisplay this help and exit\n"
	     << "\t-v\tdisplay version information and exit\n";
}
