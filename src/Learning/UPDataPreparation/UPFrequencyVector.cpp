// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPFrequencyVector.h"

/////////////////////////////////////////////////////////////////////
/* int KWFrequencyVectorModalityNumberCompare(const void* elem1, const void* elem2)
{
	KWFrequencyVector* frequencyVector1;
	KWFrequencyVector* frequencyVector2;

	frequencyVector1 = cast(KWFrequencyVector*, *(Object**)elem1);
	frequencyVector2 = cast(KWFrequencyVector*, *(Object**)elem2);

	// Comparaison du nombre de modalites par valeurs decroissantes
	return (frequencyVector2->GetModalityNumber() - frequencyVector1->GetModalityNumber());
}*/

////////////////////////////////////////////////////////////////////
// Classe KWDenseFrequencyVector

int UPDenseFrequencyVector::GetSize() const
{
	return ivFrequencyVector.GetSize();
}

int UPDenseFrequencyVector::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int i;

	// Cumul des effectifs du vecteur
	nTotalFrequency = 0;
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		nTotalFrequency += ivFrequencyVector.GetAt(i);
	return nTotalFrequency;
}

////////////////////////////////////////////////////////////////////
// Classe UPFrequencyTable

UPFrequencyTable::UPFrequencyTable()
{
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new UPDenseFrequencyVector;
	nTotalFrequency = -1;
	nGranularity = 0;
	nGarbageModalityNumber = 0;
	nInitialValueNumber = 0;
	nGranularizedValueNumber = 0;
	nTargetModalityNumber = 1;
	nTreatementModalityNumber = 1;
}

UPFrequencyTable::~UPFrequencyTable() {}
/* {
	oaFrequencyVectors.DeleteAll();

	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;
}*/

void UPFrequencyTable::CopyFrom(const KWFrequencyTable* kwftOriSource)
{

	KWFrequencyTable::CopyFrom(kwftOriSource);
	//NV
	UPFrequencyTable* kwftSource = cast(UPFrequencyTable*, kwftOriSource);
	SetTargetModalityNumber(kwftSource->GetTargetModalityNumber());
	SetTreatementModalityNumber(kwftSource->GetTreatementModalityNumber());
	nTargetModalityNumber = cast(UPFrequencyTable*, kwftOriSource)->nTargetModalityNumber;
	nTreatementModalityNumber = cast(UPFrequencyTable*, kwftOriSource)->nTreatementModalityNumber;
}

KWFrequencyTable* UPFrequencyTable::Clone() const
{
	UPFrequencyTable* kwftClone;

	kwftClone = new UPFrequencyTable;
	kwftClone->nTargetModalityNumber = nTargetModalityNumber;
	kwftClone->nTreatementModalityNumber = nTreatementModalityNumber;
	kwftClone->CopyFrom(this);
	return kwftClone;
}

void UPFrequencyTable::Write(ostream& ost) const
{
	int i;
	KWFrequencyVector* kwfvCurrent;

	ost << "Initial value number\t" << nInitialValueNumber << "\tGranularized value number\t"
	    << nGranularizedValueNumber << endl;
	// Affichage de la granularite et de la taille du groupe poubelle
	ost << "Granularity\t" << nGranularity << "\tGarbage modality number\t" << nGarbageModalityNumber << "\n";

	// Affichage de la table si non vide
	assert(oaFrequencyVectors.GetSize() == 0 or kwfvFrequencyVectorCreator != NULL);
	for (i = 0; i < GetFrequencyVectorNumber(); i++)
	{
		kwfvCurrent = GetFrequencyVectorAt(i);

		// Affichage du vecteur d'effectif
		if (i == 0)
		{
			ost << "Index\t";
			kwfvCurrent->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		ost << i << "\t";
		kwfvCurrent->WriteLineReport(ost);
		ost << "\n";
	}
}
