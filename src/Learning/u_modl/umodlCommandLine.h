// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Version.h"

////////////////////////////////////////////////////////////////
// Classe UMODLCommandLine
// Lancement du calcul de l'analyse d'un probleme d'uplift depuis la ligne de commande
class UMODLCommandLine : public Object
{
public:
	// classe d'agregation des arguments passes en ligne de commande
	class Arguments : public Object
	{
	public:
		ALString dataFileName;       // nom du fichier contenant la base de donnees
		ALString domainFileName;     // nom du fichier dictionnaire .kdic
		ALString className;          // nom de la classe d'interet dans le dictionnaire
		ALString attribTreatName;    // nom de l'attribut traitement du probleme d'uplift
		ALString attribTargetName;   // nom de l'attribut cible du probleme d'uplift
		ALString outputFileName;     // nom du fichier pour l'ecriture du dictionnaire recode
		ALString reportJSONFileName; // nom du fichier .json pour l'ecriture des statistiques calculees
		int maxPartNumber;       // nombre max d'intervalles ou de groupes (optionnel, 2 par défaut)
	};

	// Initialisation des parametres
	// Renvoie true si ok, en parametrant le nom du fichier de valeur
	// et du fichier dictionnaire en entree
	bool InitializeParameters(int argc, char** argv, Arguments& res);

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	/////////////////////////////////////////////////////////
	// Affichage de l'aide
	void ShowHelp();
};
