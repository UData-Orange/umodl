// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWAttributeStats.h"
#include "KWContinuous.h"
#include "KWFrequencyVector.h"
#include "KWSymbol.h"

inline int SearchSymbol(const SymbolVector& sv, const Symbol& s)
{
	const int size = sv.GetSize();
	for (int i = 0; i < size; i++)
	{
		if (not s.Compare(sv.GetAt(i))) // renvoie 0 si symboles egaux
		{
			return i;
		}
	}
	return size;
}

inline int SearchContinuous(const ContinuousVector& cv, const Continuous s)
{
	const int size = cv.GetSize();
	for (int i = 0; i < size; i++)
	{
		if (cv.GetAt(i) == s) // renvoie 0 si symboles egaux
		{
			return i;
		}
	}
	return size;
}

inline IntVector* GetDenseVectorAt(KWFrequencyTable& fTable, int idx)
{
	return cast(KWDenseFrequencyVector*, fTable.GetFrequencyVectorAt(idx))->GetFrequencyVector();
}

inline int GetValueNumber(KWAttributeStats& attStats)
{
	return attStats.GetDescriptiveStats()->GetValueNumber();
}

////////////////////////////////////////////////////////////////////////
// fonction d'aide a la creation d'un nouvel attribut de type categoriel
// a partir de la concatenation de deux autres attributs categoriels
KWAttribute* MakeConcatenatedAttribute(KWClass& dico, const ALString& attribOperand1, const ALString attribOperand2);

////////////////////////////////////////////////////////////////////////
// adaptation de BuildRecodingClass pour un parametre d'entree de type ObjectArray
// parametre attribStats est un object array de KWAttributeStats
void BuildRecodingClass(const KWClassDomain* initialDomain, ObjectArray* const attribStats,
			KWClassDomain* const trainedClassDomain);
