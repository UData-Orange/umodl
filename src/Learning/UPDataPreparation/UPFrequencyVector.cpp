// Copyright (c) 2025 Orange. All rights reserved.
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
// Classe KWFrequencyTable
