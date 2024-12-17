// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHHistogramSpec.h"
#include "MHHistogramSpecView.h"
#include "MHDiscretizerTruncationMODLHistogram.h"
#include "MHBin.h"
#include "JSONFile.h"
#include "Version.h"

////////////////////////////////////////////////////////////////
// Classe UMODLCommandLine
// Lancement du calcul d'un histogramme de puis la ligne de commande
class UMODLCommandLine : public Object
{
public:
	class Arguments : public Object
	{
	public:
		ALString dataFileName;
		ALString domainFileName;
		ALString className;
		ALString attribTreatName;
		ALString attribTargetName;
		ALString outputFileName;
		ALString reportJSONFileName;
	};

	// Lancement depuis la ligne de commande
	boolean ComputeHistogram(int argc, char** argv);

	// Initialisation des parametres
	// Renvoie true si ok, en parametrant le nom du fichier de valeur
	// et du fichier dictionnaire en entree
	bool InitializeParameters(int argc, char** argv, Arguments& res);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	const ALString& GetDataFileName() const;
	const ALString& GetDomainFileName() const;
	const ALString& GetClassName() const;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	/////////////////////////////////////////////////////////
	// Export des resultats au format csv
	// Renvoie true si pas d'erreur

	// Export d'un histogramme dans un fichier au format csv
	boolean ExportHistogramStatsCsvReportFile(const MHMODLHistogramStats* histogramStats,
						  const ALString& sFileName);

	// Export des stats sur l'ensemble des histogrammes au format csv, dans une serie de fichiers dont le nom est
	// deduit d'un nom de base
	boolean ExportHistogramAnalysisStastCsvReportFiles(const MHMODLHistogramAnalysisStats* histogramAnalysisStats,
							   const ALString& sBaseFileName);

	/////////////////////////////////////////////////////////
	// Export des resultats dans un fichier json
	// Renvoie true si pas d'erreur

	// Export d'un histogramme dans un fichier json
	// L'histogramme est sous la forme d'un objet, dont la cle est optionnelle
	boolean ExportHistogramStatsJson(const MHMODLHistogramStats* histogramStats, const ALString& sKey,
					 JSONFile* jsonFile);

	// Export des stats sur l'ensemble des histogrammes au format json
	boolean ExportHistogramAnalysisStastJsonReportFile(const MHMODLHistogramAnalysisStats* histogramAnalysisStats,
							   const ALString& sFileName);

	// Lecture des couples (value, frequency) sous la forme de vecteurs
	// Les vecteurs crees selon le format sont:
	//   . format 1: UpperValues
	//   . format 2: UpperValues, Frequencies
	//   . format 3: LowerValues, UpperValues, Frequencies
	// Les autres vecteur sont mis a NULL
	// Renvoie true si ok
	boolean ReadBins(ContinuousVector*& cvLowerValues, ContinuousVector*& cvUpperValues, IntVector*& ivFrequencies);

	// Lecture des valeur sous la forme de vecteur
	// Renvoie true si ok
	boolean ReadValues(ContinuousVector*& cvValues);

	// Lecture d'un champ de type valeur
	boolean ReadValue(InputBufferedFile* inputFile, longint lRecordIndex, Continuous& cValue, boolean& bEndOfLine);

	// Lecture d'un champ de type effectif
	boolean ReadFrequency(InputBufferedFile* inputFile, longint lRecordIndex, longint& lCumulatedFrequency,
			      int& nFrequency, boolean& bEndOfLine);

	// Emission d'un warning ou d'un erreur de lecture de fichier pour un numero de ligne donne
	void AddInputFileWarning(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);
	void AddInputFileError(InputBufferedFile* inputFile, longint lRecordIndex, const ALString& sLabel);

	// Affichage de l'aide
	void ShowHelp();

	// Nom du fichier de donnees
	ALString sDataFileName;

	// Nom du fichier de dictionnaire
	ALString sClassFileName;

	// Nom du fichier contenant l'histogramme resultat
	ALString sHistogramFileName;

	// Nom de la classe a lire dans le dictionnaire
	ALString sClassName;

	// Indicateur de l'option d'analyse exploratoire
	boolean bExploratoryAnalysis = false;

	// Indicateur de l'option de format json
	boolean bJsonFormat = false;

	// Objet de parametrage des histogrammes
	MHHistogramSpec histogramSpec;
};
