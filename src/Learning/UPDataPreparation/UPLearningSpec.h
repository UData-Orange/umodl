// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class UPLearningSpec;

#include "KWLearningSpec.h"

//////////////////////////////////////////////////
// Classe ULLearningSpec
// Specification d'un probleme d'apprentissage
// Services de base
class UPLearningSpec : public KWLearningSpec
{
public:
	// Constructeur et destructeur
	UPLearningSpec();
	~UPLearningSpec();

	// Attribut Treatement
	// Si un attribut cible est specifie, on est dans le cas de
	// l'apprentissage supervise
	// Provoque la reinitialisation des statistiques sur l'eventuel attribut cible
	void SetTreatementAttributeName(const ALString& sValue);
	const ALString& GetTreatementAttributeName() const;

	// Type de l'attribut cible (Continuous, Symbol, ou None en nom supervise)
	// Automatiquement synchronise avec la classe et le nom de l'attribut cible
	int GetTreatementAttributeType() const;

	// Modalite principale de l'attribut cible
	// Utile dans le cas d'un attribut cible booleenne pour presenter tous les
	// resultats par rapport a une unique valeur de reference
	// Si valeur non trouvee parmi es valeurs cibles, option ignoree
	void SetMainTreatementModality(const Symbol& sValue);
	Symbol& GetMainTreatementModality() const;

	///////////////////////////////////////////////////////////////////////////////
	// Calcul de statistiques descriptives sur l'attribut cible

	// Calcul des statistiques de l'attribut Treatement (descriptives et des valeurs)
	// La table de tuples en parametre peut ne contenir aucun attribut le cas non supervise
	// (mais donne acces a l'effectif de la base), et contient l'attribut cible en premiere position sinon
	boolean ComputeTreatementStats(const KWTupleTable* treatementTupleTable);

	// Indique si les stats sont calculees
	// La modification des specification d'apprentissage (base, dictionnaire, attribut cible...)
	// invalide la calcul des stats
	boolean IsTreatementStatsComputed() const;

	// Statistique descriptives de l'attribut Treatement
	// Retourne une objet KWDescriptiveContinuousStats ou KWDescriptiveSymbolStats, ou null
	// selon le type de l'attribut
	// Automatiquement synchronise avec le type de l'attribut Treatement
	// Memoire: appartient aux specifications
	KWDescriptiveStats* GetTreatementDescriptiveStats() const;

	// Statistique des valeurs de l'attribut Treatement
	// Retourne un objet KWDataGridsStats univarie dans le cas supervise, null sinon
	// Memoire: appartient aux specifications
	KWDataGridStats* GetTreatementValueStats() const;

	// Index de la modalite principale dans les valeurs Treatement cibles (-1 eventuellement)
	int GetMainTreatementModalityIndex() const;

	////////////////////////////////////////////////////////////////////
	// Parametrage avance

	///////////////////////////////////////////////////////////////////
	// Fonctionnalites standard

	// Duplication
	// Les objets references Class et Database sont les memes, le reste est duplique
	UPLearningSpec* Clone() const;

	// Recopie
	void CopyFrom(const UPLearningSpec* kwlsSource);

	// Recopie uniquement des statistiques sur la cible
	// Attention, methode avancee pouvant entrainer des incoherences
	// Permet de rappatrier ces infos, si elles ont ete invalidee par parametrage du dictionnaire ou de la database
	void CopyTreatementStatsFrom(const UPLearningSpec* kwlsSource);

	// Verification de la validite de la definition
	boolean Check() const override;

	// Parametrage de la verification ou non de la presence de l'attribut Treatement dans la classe (defaut: true)
	// Methode avancee, utile si l'on a deja collecte les stats sur l'attribut cible, mais que l'on en
	// a plus besoin de cet attribut pour l'analyse des attributs descriptifs
	void SetCheckTreatementAttribute(boolean bValue);
	boolean GetCheckTreatementAttribute() const;

	// Libelles
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
	void InitFrequencyTable(KWFrequencyTable* kwftSource);
	void InitFrequencyVector(const KWFrequencyVector* kwfvVector);
	boolean CheckFrequencyTable(KWFrequencyTable* kwftSource);
	boolean CheckFrequencyVector(const KWFrequencyVector* kwfvVector);
	int nTreatementModalityNumber;
	int nTargetModalityNumber;

protected:
	// Reinitialisation des statistiques descriptives
	void ResetTreatementStats();
	void ComputeNullCost();
	// Calcul de l'index de la valeur cible principale (-1 si aucune)
	// parmi les valeurs cibles
	int ComputeMainTreatementModalityIndex() const;

	// Attributs principaux

	ALString sTreatementAttributeName;
	mutable Symbol sMainTreatementModality;
	int nTreatementAttributeType;
	boolean bIsTreatementStatsComputed;
	boolean bCheckTreatementAttribute;

	KWDescriptiveStats* treatementDescriptiveStats;
	KWDataGridStats* treatementValueStats;
	int nMainTreatementModalityIndex;

	friend class PLShared_LearningSpec;
};

///////////////////////////////////////////////////////////
// Implementation en inline

///////////////////////////////////////////////////

inline const ALString& UPLearningSpec::GetTreatementAttributeName() const
{
	return sTreatementAttributeName;
}

inline int UPLearningSpec::GetTreatementAttributeType() const
{
	return nTreatementAttributeType;
}

inline void UPLearningSpec::SetMainTreatementModality(const Symbol& sValue)
{
	sMainTreatementModality = sValue;
	nMainTreatementModalityIndex = ComputeMainTreatementModalityIndex();
}

inline Symbol& UPLearningSpec::GetMainTreatementModality() const
{
	return sMainTreatementModality;
}

inline boolean UPLearningSpec::IsTreatementStatsComputed() const
{
	return bIsTreatementStatsComputed;
}

inline KWDescriptiveStats* UPLearningSpec::GetTreatementDescriptiveStats() const
{
	require(bIsTreatementStatsComputed);
	return treatementDescriptiveStats;
}

inline KWDataGridStats* UPLearningSpec::GetTreatementValueStats() const
{
	require(bIsTreatementStatsComputed);
	return treatementValueStats;
}

inline int UPLearningSpec::GetMainTreatementModalityIndex() const
{
	require(bIsTreatementStatsComputed);
	return nMainTreatementModalityIndex;
}
