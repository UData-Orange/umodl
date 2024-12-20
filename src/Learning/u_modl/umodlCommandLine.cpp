// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UmodlCommandLine.h"

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
				  << "Copyright (C) 2024 Orange labs\n";
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
	if (argc != 7 and argc != 8)
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
	res.outputFileName = argv[6];
	if (res.dataFileName == res.domainFileName or res.dataFileName == res.outputFileName or
	    res.domainFileName == res.outputFileName)
	{
		std::cout << "All file names must be different.\n";
		return false;
	}

	if (res.attribTreatName == res.attribTargetName)
	{
		std::cout << "Treatment and Target attributes must be different.\n";
		return false;
	}

	// preparation du nom de fichier pour le rapport json
	ALString& reportFileNameToCheck = (argc == 8) ? res.reportJSONFileName : res.domainFileName;
	if (argc == 8)
	{
		res.reportJSONFileName = argv[7];
	}
	//verification de l'extension
	const int extPos = reportFileNameToCheck.ReverseFind('.');
	if (extPos <= 0)
	{
		AddError("Cannot create JSON report, filename without consistent extension.");
		return false;
	}
	// verification de l'extension : .json attendu si nom de fichier en argument,
	// .kdic pour le dictionnaire si pas de nom de fichier pour le rapport
	const ALString fileExt = reportFileNameToCheck.Right(reportFileNameToCheck.GetLength() - extPos);
	if ((&reportFileNameToCheck == &(res.domainFileName) and fileExt != ".kdic") or
	    (&reportFileNameToCheck == &(res.reportJSONFileName) and fileExt != ".json"))
	{
		AddError("Cannot create JSON report, inconsistent file extension.");
		return false;
	}
	// modification du nom si pas d'argument pour le nom du fichier de rapport
	if (argc == 7)
	{
		res.reportJSONFileName = res.domainFileName.Left(extPos) + ".json";
	}

	return true;
}

void UMODLCommandLine::ShowHelp()
{
	cout << "Usage: " << GetClassLabel()
	     << " [VALUES] [DICTIONNARY] [CLASS] [TREATMENT] [TARGET] [RECODED KDIC] [REPORT JSON]\n ";
	// cout << "Compute uplift statistics from the data in VALUES.\n";
	// cout << "A recoded dictionnary is output in OUTPUT file, with the lower bound, upper bound,\n";
	// cout << "  length, frequency, probability and density per bin.\n ";
	// cout << "A report of the statistics of the variables is output as a JSON file in RECODED.json or in REPORT.json if the argument is passed to the program.\n";

	// Options generales
	cout << "\t-h\tdisplay this help and exit\n";
	cout << "\t-v\tdisplay version information and exit\n";

	// Aide additionnelle
	// cout << "\n";
	// cout << "The output histogram is as accurate and interpretable as possible.\n";
	// cout << " Each histogram of the series uses an index in its suffix(e.g. \".1\"), and an additional file\n";
	// cout << " with the suffix \".series\" is produced, with indicators per histogram.\n";
	// cout << "The -j option can be combined with the -e option to get all outputs in one file.\n";
}
