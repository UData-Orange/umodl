// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class UPMODLDiscretizationCosts;
class UPMODLGroupingCosts;

#include "KWStat.h"
#include "KWUnivariatePartitionCost.h"
#include "UPFrequencyVector.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariatePartitionCosts
// Definition de la structure des couts d'une partition univariee des donnees
// Les couts par entite, nuls par defaut, sont redefinissable dans des sous-classes
// La partition est definie par le nombre de parties. Chaque partie est representee
// par un vecteur d'effectifs (sous-classe de KWFrequencyVector).
// Des parametres globaux (nombre de valeurs total...) peuvent egalement etre
// utilise dans des sous-classes, afin de parametrer les couts

////////////////////////////////////////////////////////////////////////////
// Classe UPMODLDiscretizationCosts
// Definition de la structure des couts d'une discretisation MODL
class UPMODLDiscretizationCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	UPMODLDiscretizationCosts();
	~UPMODLDiscretizationCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const override;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe UPMODLGroupingCosts
// Definition de la structure des couts d'une discretisation MODL
class UPMODLGroupingCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	UPMODLGroupingCosts();
	~UPMODLGroupingCosts();

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	// Le parametre nPartNumber designe le nombre total de parties
	// Une partition avec groupe poubelle non vide contient donc une partition en nPartNumber-1 groupes + 1 groupe
	// poubelle
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;

	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionConstructionCost(int nPartNumber) const override;
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Classe UPUnivariateNullPartitionCosts
// Variante d'une structure de cout, en ignorant les cout de partition
class UPUnivariateNullPartitionCosts : public KWUnivariatePartitionCosts
{
public:
	// Constructeur
	UPUnivariateNullPartitionCosts();
	~UPUnivariateNullPartitionCosts();

	// Parametrage d'une structure de cout, pour reprendre les cout de partie,
	// mais ignorer els cout de partition
	// Par defaut: NULL (tous les couts a 0)
	// Memoire: l'objet appartient a l'appele (le Set remplace et detruit le parametre precedent)
	void SetUnivariatePartitionCosts(KWUnivariatePartitionCosts* kwupcCosts);
	KWUnivariatePartitionCosts* GetUnivariatePartitionCosts() const;

	//////////////////////////////////////////////////////////////
	// Redefinition des methodes virtuelles

	// Creation
	KWUnivariatePartitionCosts* Create() const override;

	// Recopie du parametrage d'un objet de la meme classe
	void CopyFrom(const KWUnivariatePartitionCosts* sourceCosts) override;

	// Redefinition des methodes de calcul de cout
	// (Les parties doivent etre de type KWDenseFrequencyVector)
	double ComputePartitionCost(int nPartNumber) const override;
	double ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber) const override;
	double ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartCost(const KWFrequencyVector* part) const override;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const override;

	// Affichage du cout de la partition
	void WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const override;

	// Cout de modele par entite
	double ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const override;
	double ComputePartModelCost(const KWFrequencyVector* part) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Couts de partitionnement univarie de reference
	KWUnivariatePartitionCosts* univariatePartitionCosts;
};
