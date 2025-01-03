// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWFrequencyVector.h"

// Comparaison des nombres de modalites des lignes de contingence
//int KWFrequencyVectorModalityNumberCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////////////////
// Vecteur d'effectifs plein
// Similaire a un compteur d'entiers, avec gestion de l'effectif total
class UPDenseFrequencyVector : public KWDenseFrequencyVector
{
public:
	// Constructeur
	UPDenseFrequencyVector();
	~UPDenseFrequencyVector();

	// Createur, renvoyant une instance du meme type
	KWFrequencyVector* Create() const override;

	// Vecteur d'effectif par classe cible
	IntVector* GetFrequencyVector();

	// Taille du vecteur d'effectif
	int GetSize() const override;

	// Calcul de l'effectif total
	int ComputeTotalFrequency() const override;

	// Copie a partir d'un vecteur source
	void CopyFrom(const KWFrequencyVector* kwfvSource) override;

	// Duplication (y compris du contenu)
	KWFrequencyVector* Clone() const override;

	// Rapport synthetique destine a rentrer dans une sous partie d'un tableau
	void WriteHeaderLineReport(ostream& ost) const override;
	void WriteLineReport(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Acces au nombre de modalites
	int GetTargetModalityNumber() const;
	void SetTargetModalityNumber(int nModality);

	// Acces au nombre de modalites
	int GetTreatementModalityNumber() const;
	void SetTreatementModalityNumber(int nModality);

	///////////////////////////////
	///// Implementation
	// Verification de l'integrite des specification

protected:
	// Vecteur de comptage des effectifs par classe cible
	//IntVector ivFrequencyVector;
	int nTargetModalityNumber;
	int nTreatementModalityNumber;
};

///////////////////////////////////////////////////////////////////////
// Methodes en inline

// Classe UPDenseFrequencyVector

inline UPDenseFrequencyVector::UPDenseFrequencyVector()
{
	nTreatementModalityNumber = 3;
	nTargetModalityNumber = 3;
}

inline UPDenseFrequencyVector::~UPDenseFrequencyVector() {}

inline KWFrequencyVector* UPDenseFrequencyVector::Create() const
{
	UPDenseFrequencyVector* vect = new UPDenseFrequencyVector;
	vect->SetTargetModalityNumber(nTargetModalityNumber);
	vect->SetTreatementModalityNumber(nTreatementModalityNumber);
	return vect;
}

inline IntVector* UPDenseFrequencyVector::GetFrequencyVector()
{
	return &ivFrequencyVector;
}

inline void UPDenseFrequencyVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const UPDenseFrequencyVector* kwdfvSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwdfvSource = cast(UPDenseFrequencyVector*, kwfvSource);

	// Recopie du vecteur d'effectifs
	ivFrequencyVector.CopyFrom(&(kwdfvSource->ivFrequencyVector));

	nTargetModalityNumber = cast(UPDenseFrequencyVector*, kwfvSource)->nTargetModalityNumber;
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, kwfvSource)->nTreatementModalityNumber;
}

inline KWFrequencyVector* UPDenseFrequencyVector::Clone() const
{
	UPDenseFrequencyVector* kwfvClone;

	kwfvClone = new UPDenseFrequencyVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

inline void UPDenseFrequencyVector::WriteHeaderLineReport(ostream& ost) const
{
	int i;

	// Libelle des valeurs cibles
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		ost << "T" << i / nTreatementModalityNumber + 1 << "_C" << i % nTreatementModalityNumber + 1 << "\t";
	ost << "V";
}

inline void UPDenseFrequencyVector::WriteLineReport(ostream& ost) const
{
	int i;

	// Frequence des valeurs cibles
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		ost << ivFrequencyVector.GetAt(i) << "\t";
	ost << nModalityNumber;
}

inline const ALString UPDenseFrequencyVector::GetClassLabel() const
{
	return "Uplift Dense frequency vector";
}

inline int UPDenseFrequencyVector::GetTargetModalityNumber() const
{
	return nTargetModalityNumber;
}

inline void UPDenseFrequencyVector::SetTargetModalityNumber(int nModality)
{
	nTargetModalityNumber = nModality;
}

inline int UPDenseFrequencyVector::GetTreatementModalityNumber() const
{
	return nTreatementModalityNumber;
}

inline void UPDenseFrequencyVector::SetTreatementModalityNumber(int nModality)
{
	nTreatementModalityNumber = nModality;
}
