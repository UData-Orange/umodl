// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
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
