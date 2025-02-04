// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPUnivariatePartitionCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariatePartitionCosts

//const double UPMODLDiscretizationCosts::dEpsilon = 1e-6;

////////////////////////////////////////////////////////////////////////////
// Classe UPMODLDiscretizationCosts

UPMODLDiscretizationCosts::UPMODLDiscretizationCosts()
{
	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;

	kwfvFrequencyVectorCreator = new UPDenseFrequencyVector;
}

UPMODLDiscretizationCosts::~UPMODLDiscretizationCosts() {}

KWUnivariatePartitionCosts* UPMODLDiscretizationCosts::Create() const
{
	return new UPMODLDiscretizationCosts;
}

double UPMODLDiscretizationCosts::ComputePartitionCost(int nPartNumber) const
{
	// Cas d'utilisation de la granularite
	double dCost;
	boolean bDisplayResults = false;
	int nGranularityMax;

	require(nClassValueNumber > 1);
	require(nPartNumber >= 1);
	require(nPartNumber <= nValueNumber);
	require(nGranularity >= 1 or nPartNumber == 1);
	require(nTotalInstanceNumber > 0);

	nGranularityMax = (int)ceil(log(GetTotalInstanceNumber() * 1.0) / log(2.0));

	// Cout choix entre modele nul et modele informatif
	dCost = log(2.0);
	if (bDisplayResults)
		cout << "Choix modele informatif " << log(2.0) << endl;

	// Cout du choix du Traitement W
	dCost += log(2.0) * nPartNumber;

	// Si modele informatif
	if (nPartNumber > 1 and nValueNumber > 1)
	{

		// Cout de selection/construction de l'attribut
		dCost += dAttributeCost;
		if (bDisplayResults)
			cout << " Cout de selection/construction de l'attribut " << dAttributeCost << endl;

		// Cout du choix de la granularite
		dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);

		if (bDisplayResults)
			cout << "Cout choix granularite " << nGranularity << " = "
			     << KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax) << endl;

		// Nombre d'intervalles
		dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1);
		if (bDisplayResults)
			cout << "Cout choix nombre de parties " << nPartNumber << " parmi " << nValueNumber << "\t"
			     << KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1)
			     << endl;

		// Partition en intervalles
		// Nouveau codage avec description du choix des coupures selon une multinomiale
		dCost += (nPartNumber - 1) * log((nValueNumber - 1) * 1.0);
		dCost -= KWStat::LnFactorial(nPartNumber - 1);

		if (bDisplayResults)
			cout << "Cout choix intervalles "
			     << KWStat::LnFactorial(nValueNumber + nPartNumber - 1) -
				    KWStat::LnFactorial(nValueNumber) - KWStat::LnFactorial(nPartNumber - 1)
			     << endl;
	}
	if (bDisplayResults)
		cout << "Cout complet avec granularite " << nGranularity << " = "
		     << " \tnPartNumber = " << nPartNumber << "\t " << dCost << endl;

	return dCost;
}

double UPMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	// Cas d'utilisation de la granularite
	require(nValueNumber > 0);
	require(nPartNumber > 1);
	require(nPartNumber <= nValueNumber);
	require(nClassValueNumber > 1);
	require(nGranularity >= 1);

	// Cas d'une partition en au moins deux intervalles
	if (nPartNumber > 2)
	{
		dDeltaCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 2, nValueNumber - 1) -
			     KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1);

		// Nouveau codage avec description du choix des coupures selon une multinomiale
		dDeltaCost = dDeltaCost + log(nPartNumber - 1.0) - log(nValueNumber - 1.0) - log(2.0);
	}
	// Sinon, on compare le cout de la partition en un intervalle au cout du modele nul (1 intervalle)
	else
		dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);

	ensure(fabs(ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber) - dDeltaCost) < dEpsilon);
	return dDeltaCost;
}

double UPMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double UPMODLDiscretizationCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost0, dCost1;
	int nFrequency;
	int nIntervalFrequency;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);
	//require(cast(UPDenseFrequencyVector*, part)->GetObjectLabel() == "Uplift Dense frequency vector");

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(UPDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost1 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost1 -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost0 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			dCost0 -= KWStat::LnFactorial(nFrequency);
			nIntervalFrequency += nFrequency;
		}
		dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	}

	return ((dCost0) < (dCost1) ? (dCost0) : (dCost1));
}

int UPMODLDiscretizationCosts::ComputePartCostW(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost0, dCost1;
	int nFrequency;
	int nIntervalFrequency;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);
	//require(cast(UPDenseFrequencyVector*, part)->GetObjectLabel() == "Uplift Dense frequency vector");

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(UPDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost1 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost1 -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost0 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			dCost0 -= KWStat::LnFactorial(nFrequency);
			nIntervalFrequency += nFrequency;
		}
		dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	}

	return ((dCost0) < (dCost1) ? 0 : 1);
}

// Calcul du cout global de la partition, definie par le tableau de ses parties
void UPMODLDiscretizationCosts::ComputePartitionGlobalCostW(const KWFrequencyTable* partTable,
							    IntVector* ivtreatementgroups)
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;
	int nPartileNumber;

	require(partTable != NULL);
	//require(nGranularity == partTable->GetGranularity());
	require(ivtreatementgroups != NULL);

	// Parametrage de la structure de cout
	nGranularity = partTable->GetGranularity();
	nTotalInstanceNumber = partTable->GetTotalFrequency();
	nPartileNumber = (int)pow(2, partTable->GetGranularity());
	if (nPartileNumber > nTotalInstanceNumber or nGranularity == 0)
		nPartileNumber = nTotalInstanceNumber;
	nValueNumber = nPartileNumber;

	nClassValueNumber = 0;
	if (partTable->GetFrequencyVectorNumber() > 0)
		nClassValueNumber =
		    cast(KWDenseFrequencyVector*, partTable->GetFrequencyVectorAt(0))->GetFrequencyVector()->GetSize();

	// calcul de W
	ivtreatementgroups->SetSize(partTable->GetFrequencyVectorNumber());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		ivtreatementgroups->SetAt(i, ComputePartCostW(partTable->GetFrequencyVectorAt(i)));
}

double UPMODLDiscretizationCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}

void UPMODLDiscretizationCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double UPMODLDiscretizationCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double UPMODLDiscretizationCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionCost(nPartNumber);
}

double UPMODLDiscretizationCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	IntVector* ivFrequencyVector;
	int nFrequency;
	int nIntervalFrequency;
	double dCost0, dCost1;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);
	//require(part->GetObjectLabel() == "Uplift Dense frequency vector");

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost0 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		nIntervalFrequency += nFrequency;
	}
	dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	dCost0 -= KWStat::LnFactorial(nIntervalFrequency);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost1 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			nIntervalFrequency += nFrequency;
		}

		dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
		dCost1 -= KWStat::LnFactorial(nIntervalFrequency);
	}

	return ((dCost0) < (dCost1) ? (dCost0) : (dCost1));
}

const ALString UPMODLDiscretizationCosts::GetClassLabel() const
{
	return "UMODL discretization costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe UPMODLGroupingCosts

UPMODLGroupingCosts::UPMODLGroupingCosts()
{
	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;

	kwfvFrequencyVectorCreator = new UPDenseFrequencyVector;
}

UPMODLGroupingCosts::~UPMODLGroupingCosts() {}

KWUnivariatePartitionCosts* UPMODLGroupingCosts::Create() const
{
	return new UPMODLGroupingCosts;
}

double UPMODLGroupingCosts::ComputePartitionCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionCost(nPartNumber, 0);
}

double UPMODLGroupingCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;
	int nInformativeValueNumber;
	int nInformativePartNumber;
	int nGranularityMax;

	require(nGranularity >= 0);
	require(nTotalInstanceNumber > 1 or nGranularity == 0);

	// Initialisations
	// Granularite maximale
	nGranularityMax = (int)ceil(log(GetTotalInstanceNumber() * 1.0) / log(2.0));
	// Nombre de valeurs informatives (hors groupe poubelle)
	nInformativeValueNumber = nValueNumber - nGarbageModalityNumber;

	// Nombre de parties informatives
	// Initialisation avec poubelle
	if (nGarbageModalityNumber > 0)
	{
		// Nombre total de parties - 1 pour le groupe poubelle
		nInformativePartNumber = nPartNumber - 1;

		// Le modele a 1 groupe + 1 groupe poubelle ne peut pas etre envisage
		assert(nInformativePartNumber > 1);
	}

	// Initialisation sans poubelle
	else
		nInformativePartNumber = nPartNumber;

	require(nInformativePartNumber <= nInformativeValueNumber);
	require(nInformativePartNumber >= 1);

	// Choix du modele nul ou modele informatif
	dCost = log(2.0);

	// Cout du choix du Traitement W
	dCost += log(2.0) * nPartNumber;

	// Si modele informatif
	if (nInformativePartNumber > 1 and nInformativeValueNumber > 1)
	{
		// Cout de selection/construction de l'attribut
		dCost += dAttributeCost;

		// Choix de la granularite si mode granu
		if (nGranularity > 0)
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);

		// Si mode poubelle et si nombre total de modalites suffisant, cout de la hierarchie poubelle
		if (nValueNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
			dCost += log(2.0);

		// Cas de l'absence de poubelle
		if (nGarbageModalityNumber == 0)
		{
			// Cout de codage du nombre de groupes
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
										  nInformativeValueNumber - 1);

			// Cout de codage du choix des groupes
			dCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber);
		}
		// Cas de la presence d'une poubelle
		else
		{
			// Cout du choix du nombre de modalites informatives hors poubelle parmi l'ensemble des
			// modalites
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativeValueNumber - 1,
										  nValueNumber - 2);

			// Cout du choix des modalites informatives parmi l'ensemble des modalites (tirage multinimial
			// avec elements distincts)
			dCost += nInformativeValueNumber * log(nValueNumber * 1.0) -
				 KWStat::LnFactorial(nInformativeValueNumber);

			// Cout de codage du nombre de groupes parmi les modalites informatives
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
										  nInformativeValueNumber - 1);

			// Cout de codage du choix des groupes
			dCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber);
		}
	}

	return dCost;
}

double UPMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionDeltaCost(nPartNumber, 0);
}
double UPMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	// Cas d'utilisation de la granularite (granularite a 0 si pas de granu)
	double dDeltaCost;
	int nInformativeValueNumber;
	int nInformativePartNumber;

	require(nGranularity >= 0);
	require(nPartNumber >= 1);
	require(nValueNumber >= nPartNumber);
	require(nValueNumber > nGarbageModalityNumber);

	// Nombre de valeurs informatives : on enleve les eventuelles modalites du groupe poubelle
	nInformativeValueNumber = nValueNumber - nGarbageModalityNumber;
	// Nombre de parties informatives
	if (nGarbageModalityNumber > 0)
		nInformativePartNumber = nPartNumber - 1;
	else
		nInformativePartNumber = nPartNumber;

	// Cas d'une partition en au moins trois groupes informatifs (soit deux groupes informatifs apres
	// decrementation)
	if (nInformativePartNumber > 2)
	{
		dDeltaCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 2,
									      nInformativeValueNumber - 1) -
			     KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
									      nInformativeValueNumber - 1);
		dDeltaCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber - 1) -
			      KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber) - log(2.0);
		;
	}
	// Sinon, on compare le cout de la partition en deux groupes informatives au cout du modele nul (1 groupe)
	else
		dDeltaCost = ComputePartitionCost(nPartNumber - 1, nGarbageModalityNumber) -
			     ComputePartitionCost(nPartNumber, nGarbageModalityNumber);

	ensure(fabs(ComputePartitionCost(nPartNumber - 1, nGarbageModalityNumber) -
		    ComputePartitionCost(nPartNumber, nGarbageModalityNumber) - dDeltaCost) < 1e-5);
	return dDeltaCost;
}

double UPMODLGroupingCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost0, dCost1;
	int nFrequency;
	int nIntervalFrequency;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(UPDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost1 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost1 -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost0 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			dCost0 -= KWStat::LnFactorial(nFrequency);
			nIntervalFrequency += nFrequency;
		}
		dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	}

	return ((dCost0) < (dCost1) ? (dCost0) : (dCost1));
}

double UPMODLGroupingCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber(), partTable->GetGarbageModalityNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}

int UPMODLGroupingCosts::ComputePartCostW(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost0, dCost1;
	int nFrequency;
	int nIntervalFrequency;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(UPDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost1 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost1 -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost0 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			dCost0 -= KWStat::LnFactorial(nFrequency);
			nIntervalFrequency += nFrequency;
		}
		dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	}

	return ((dCost0) < (dCost1) ? 0 : 1);
}

void UPMODLGroupingCosts::ComputePartitionGlobalCostW(const KWFrequencyTable* partTable,
						      IntVector* ivtreatementgroups) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(ivtreatementgroups != NULL);
	// calcul de W
	ivtreatementgroups->SetSize(partTable->GetFrequencyVectorNumber());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber(), partTable->GetGarbageModalityNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		ivtreatementgroups->SetAt(i, ComputePartCostW(partTable->GetFrequencyVectorAt(i)));
}

void UPMODLGroupingCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber, nGarbageModalityNumber)
	    << "\n";
}

double UPMODLGroupingCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double UPMODLGroupingCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return ComputePartitionCost(nPartNumber, nGarbageModalityNumber);
}

double UPMODLGroupingCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	IntVector* ivFrequencyVector;
	int nFrequency;
	int nIntervalFrequency;
	double dCost0, dCost1;
	int nTreatementModalityNumber;
	int nTargetModalityNumber;
	int i, j;

	require(part != NULL);
	require(nClassValueNumber > 1);
	//require(part->GetObjectLabel() == "Uplift Dense frequency vector");

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();
	nTreatementModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTreatementModalityNumber();
	nTargetModalityNumber = cast(UPDenseFrequencyVector*, part)->GetTargetModalityNumber();
	require(nTreatementModalityNumber > 1);
	require(ivFrequencyVector->GetSize() == nTreatementModalityNumber * nTargetModalityNumber);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=1
	dCost0 = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		nIntervalFrequency += nFrequency;
	}
	dCost0 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
	dCost0 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
	dCost0 -= KWStat::LnFactorial(nIntervalFrequency);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne W=0

	dCost1 = 0;

	for (i = 0; i < nTreatementModalityNumber; i++)
	{
		nIntervalFrequency = 0;
		for (j = 0; j < nTargetModalityNumber; j++)
		{
			nFrequency = ivFrequencyVector->GetAt(j + i * nTargetModalityNumber);
			nIntervalFrequency += nFrequency;
		}

		dCost1 += KWStat::LnFactorial(nIntervalFrequency + nTargetModalityNumber - 1);
		dCost1 -= KWStat::LnFactorial(nTargetModalityNumber - 1);
		dCost1 -= KWStat::LnFactorial(nIntervalFrequency);
	}

	return ((dCost0) < (dCost1) ? (dCost0) : (dCost1));
}

const ALString UPMODLGroupingCosts::GetClassLabel() const
{
	return "UMODL grouping costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe UPUnivariateNullPartitionCosts

UPUnivariateNullPartitionCosts::UPUnivariateNullPartitionCosts()
{
	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;

	kwfvFrequencyVectorCreator = new UPDenseFrequencyVector;
	univariatePartitionCosts = NULL;
}

UPUnivariateNullPartitionCosts::~UPUnivariateNullPartitionCosts()
{
	if (univariatePartitionCosts != NULL)
		delete univariatePartitionCosts;
}

void UPUnivariateNullPartitionCosts::SetUnivariatePartitionCosts(KWUnivariatePartitionCosts* kwupcCosts)
{
	if (univariatePartitionCosts != NULL)
		delete univariatePartitionCosts;
	univariatePartitionCosts = kwupcCosts;
}

KWUnivariatePartitionCosts* UPUnivariateNullPartitionCosts::GetUnivariatePartitionCosts() const
{
	return univariatePartitionCosts;
}

KWUnivariatePartitionCosts* UPUnivariateNullPartitionCosts::Create() const
{
	return new UPUnivariateNullPartitionCosts;
}

void UPUnivariateNullPartitionCosts::CopyFrom(const KWUnivariatePartitionCosts* sourceCosts)
{
	const UPUnivariateNullPartitionCosts* sourceNullCost;

	require(sourceCosts != NULL);

	// Acces a'lobjet source dans son bon type
	sourceNullCost = cast(const UPUnivariateNullPartitionCosts*, sourceCosts);

	// Appel de la methode ancetre
	KWUnivariatePartitionCosts::CopyFrom(sourceNullCost);

	// Recopie du cout de base
	if (sourceNullCost != this)
		SetUnivariatePartitionCosts(sourceNullCost->GetUnivariatePartitionCosts());
}

double UPUnivariateNullPartitionCosts::ComputePartitionCost(int nPartNumber) const
{
	return 0;
}

double UPUnivariateNullPartitionCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return 0;
}

double UPUnivariateNullPartitionCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	return 0;
}

double UPUnivariateNullPartitionCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return 0;
}

double UPUnivariateNullPartitionCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->ComputePartCost(part);
	else
		return 0;
}

double UPUnivariateNullPartitionCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	double dCost;
	int i;

	require(partTable != NULL);
	require(univariatePartitionCosts->GetGranularity() == partTable->GetGranularity());
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}
void UPUnivariateNullPartitionCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double UPUnivariateNullPartitionCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return ComputePartitionCost(nPartNumber);
}

double UPUnivariateNullPartitionCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->ComputePartModelCost(part);
	else
		return 0;
}

const ALString UPUnivariateNullPartitionCosts::GetClassLabel() const
{
	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->GetClassLabel() + " (No partition cost)";
	else
		return KWUnivariatePartitionCosts::GetClassLabel() + " (No partition cost)";
}
