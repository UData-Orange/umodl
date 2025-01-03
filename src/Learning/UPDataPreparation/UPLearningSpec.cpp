// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPLearningSpec.h"

// Les include sont utiles sont dans l'implementation pour resoudre un probleme de dependance cyclique
#include "KWFrequencyVector.h"
#include "KWDiscretizerMODL.h"
#include "KWDataGrid.h"
#include "KWDescriptiveStats.h"
#include "KWDataGridCosts.h"
#include "UPFrequencyVector.h"
#include "UPDiscretizerUMODL.h"

//////////////////////////////////////////////////

UPLearningSpec::UPLearningSpec()
{
	kwcClass = NULL;
	database = NULL;
	nTreatementAttributeType = KWType::Unknown;
	bIsTreatementStatsComputed = false;
	nInstanceNumber = 0;
	treatementDescriptiveStats = NULL;
	treatementValueStats = NULL;
	nMainTreatementModalityIndex = -1;
	dNullConstructionCost = 0;
	dNullPreparationCost = 0;
	dNullDataCost = 0;
	nInitialAttributeNumber = -1;
	bMultiTableConstruction = false;
	bTextConstruction = false;
	bTrees = false;
	bAttributePairs = false;
	nConstructionFamilyNumber = 0;
	bCheckTreatementAttribute = true;

	//NV
	// Attributs principaux

	nTreatementAttributeType = KWType::Unknown;
	bIsTreatementStatsComputed = false;
	treatementDescriptiveStats = NULL;
	treatementValueStats = NULL;
	nMainTreatementModalityIndex = -1;
	nTreatementModalityNumber = 0;
	nTargetModalityNumber = 0;
}

UPLearningSpec::~UPLearningSpec()
{
	if (treatementDescriptiveStats != NULL)
		delete treatementDescriptiveStats;
	if (treatementValueStats != NULL)
		delete treatementValueStats;
}

void UPLearningSpec::SetTreatementAttributeName(const ALString& sValue)
{
	KWAttribute* treatementAttribute;

	// Memorisation du nom
	sTreatementAttributeName = sValue;

	// Synchronisation du type de l'attribut cible
	nTreatementAttributeType = KWType::Unknown;
	if (sTreatementAttributeName == "")
		nTreatementAttributeType = KWType::None;
	else if (kwcClass != NULL)
	{
		treatementAttribute = kwcClass->LookupAttribute(sTreatementAttributeName);
		if (treatementAttribute != NULL)
			nTreatementAttributeType = treatementAttribute->GetType();
	}

	// Reinitialisation des statistiques descriptives
	ResetTreatementStats();
}

boolean UPLearningSpec::ComputeTreatementStats(const KWTupleTable* tupleTable)
{
	require(Check());
	require(tupleTable != NULL);

	// Initialisation
	ResetTreatementStats();
	bIsTreatementStatsComputed = true;

	// Memorisation du nombre d'instances
	nInstanceNumber = tupleTable->GetTotalFrequency();

	// Pas de calcul de la cas non supervise
	if (GetTreatementAttributeName() == "")
	{
		assert(treatementDescriptiveStats == NULL);
		assert(treatementValueStats == NULL);
	}
	// Cas supervise
	else
	{
		assert(treatementDescriptiveStats != NULL);
		assert(treatementValueStats != NULL);
		assert(GetTreatementAttributeType() == KWType::Continuous or
		       GetTreatementAttributeType() == KWType::Symbol);

		// Calcul des stats descriptives de l'attribut cible
		bIsTreatementStatsComputed = treatementDescriptiveStats->ComputeStats(tupleTable);

		// Nettoyage initial des statistiques par valeur
		treatementValueStats->DeleteAll();

		// Calcul des statistiques par valeur
		if (bIsTreatementStatsComputed)
		{
			// Cas continu
			if (GetTreatementAttributeType() == KWType::Continuous)
			{
				bIsTreatementStatsComputed = ComputeContinuousValueStats(
				    GetTreatementAttributeName(), tupleTable, treatementValueStats);
			}
			// Cas symbol
			else if (GetTreatementAttributeType() == KWType::Symbol)
			{
				bIsTreatementStatsComputed = ComputeSymbolValueStats(
				    GetTreatementAttributeName(), tupleTable, false, treatementValueStats);

				// Index de la valeur cible principale
				if (bIsTreatementStatsComputed)
				{
					// Calcul de l'index de la valeur cible principale
					nMainTreatementModalityIndex = ComputeMainTreatementModalityIndex();

					// Parametrage de la modalite cible principale dans la grille de valeurs cibles
					treatementValueStats->SetMainTargetModalityIndex(nMainTreatementModalityIndex);
				}
			}
		}
	}

	// Calcul des cout de model null
	//if (bIsTreatementStatsComputed)
	//{
	//	ComputeNullCost();
	//}

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		ResetTreatementStats();

	// Quelques verifications
	ensure(treatementDescriptiveStats == NULL or treatementValueStats != NULL);
	ensure(treatementDescriptiveStats != NULL or treatementValueStats == NULL);
	ensure(treatementDescriptiveStats == NULL or not bIsTreatementStatsComputed or
	       treatementDescriptiveStats->GetValueNumber() == treatementValueStats->ComputeTotalGridSize());
	ensure(treatementValueStats == NULL or not bIsTreatementStatsComputed or
	       treatementValueStats->ComputeGridFrequency() == nInstanceNumber);
	return bIsTreatementStatsComputed;
}

UPLearningSpec* UPLearningSpec::Clone() const
{
	UPLearningSpec* kwlsClone;

	kwlsClone = new UPLearningSpec;
	kwlsClone->CopyFrom(this);
	return kwlsClone;
}

void UPLearningSpec::CopyFrom(const UPLearningSpec* kwlsSource)
{
	require(kwlsSource != NULL);

	// Recopie du contenu de l'objet source
	sShortDescription = kwlsSource->sShortDescription;
	kwcClass = kwlsSource->kwcClass;
	database = kwlsSource->database;
	sTreatementAttributeName = kwlsSource->sTreatementAttributeName;
	nTreatementAttributeType = kwlsSource->nTreatementAttributeType;
	sMainTreatementModality = kwlsSource->sMainTreatementModality;
	preprocessingSpec.CopyFrom(&kwlsSource->preprocessingSpec);

	// Recopie des donnees calculees
	nInstanceNumber = kwlsSource->nInstanceNumber;
	if (treatementDescriptiveStats != NULL)
		delete treatementDescriptiveStats;
	treatementDescriptiveStats = NULL;
	if (kwlsSource->treatementDescriptiveStats != NULL)
	{
		treatementDescriptiveStats = kwlsSource->treatementDescriptiveStats->Clone();
		treatementDescriptiveStats->SetLearningSpec(this);
	}
	if (treatementValueStats != NULL)
		delete treatementValueStats;
	treatementValueStats = NULL;
	if (kwlsSource->treatementValueStats != NULL)
		treatementValueStats = kwlsSource->treatementValueStats->Clone();
	nMainTreatementModalityIndex = kwlsSource->nMainTreatementModalityIndex;
	dNullConstructionCost = kwlsSource->dNullConstructionCost;
	dNullPreparationCost = kwlsSource->dNullPreparationCost;
	dNullDataCost = kwlsSource->dNullDataCost;
	nInitialAttributeNumber = kwlsSource->nInitialAttributeNumber;
	bMultiTableConstruction = kwlsSource->bMultiTableConstruction;
	bTextConstruction = kwlsSource->bTextConstruction;
	bTrees = kwlsSource->bTrees;
	bAttributePairs = kwlsSource->bAttributePairs;
	bIsTreatementStatsComputed = kwlsSource->bIsTreatementStatsComputed;
	bCheckTreatementAttribute = kwlsSource->bCheckTreatementAttribute;
}

void UPLearningSpec::CopyTreatementStatsFrom(const UPLearningSpec* kwlsSource)
{
	require(kwlsSource != NULL);

	// Recopie des donnees calculees
	nInstanceNumber = kwlsSource->nInstanceNumber;
	if (treatementDescriptiveStats != NULL)
		delete treatementDescriptiveStats;
	treatementDescriptiveStats = NULL;
	if (kwlsSource->treatementDescriptiveStats != NULL)
	{
		treatementDescriptiveStats = kwlsSource->treatementDescriptiveStats->Clone();
		treatementDescriptiveStats->SetLearningSpec(this);
	}
	if (treatementValueStats != NULL)
		delete treatementValueStats;
	treatementValueStats = NULL;
	if (kwlsSource->treatementValueStats != NULL)
		treatementValueStats = kwlsSource->treatementValueStats->Clone();
	nMainTreatementModalityIndex = kwlsSource->nMainTreatementModalityIndex;
	bIsTreatementStatsComputed = kwlsSource->bIsTreatementStatsComputed;
	bCheckTreatementAttribute = kwlsSource->bCheckTreatementAttribute;
}

boolean UPLearningSpec::Check() const
{
	boolean bOk = true;

	// Classe
	if (kwcClass == NULL)
	{
		bOk = false;
		AddError("Missing dictionary");
	}
	else if (not kwcClass->Check())
	{
		bOk = false;
		AddError("Incorrect dictionary");
	}

	// Base de donnees
	if (database == NULL)
	{
		bOk = false;
		AddError("Missing database");
	}
	else if (kwcClass != NULL and kwcClass->GetName() != database->GetClassName())
	{
		bOk = false;
		AddError("Database (" + database->GetClassName() + ") inconsistent with dictionary (" +
			 kwcClass->GetName() + ")");
	}
	else if (not database->Check())
		bOk = false;

	// Attribut cible, si la verification est demandee
	if (bCheckTreatementAttribute and sTreatementAttributeName != "")
	{
		if (kwcClass != NULL)
		{
			KWAttribute* attribute;

			// Recherche de l'attribut et verifications
			attribute = kwcClass->LookupAttribute(sTreatementAttributeName);
			if (attribute == NULL)
			{
				bOk = false;
				AddError("Treatement variable " + sTreatementAttributeName + " unknown in dictionary " +
					 kwcClass->GetName());
			}
			else if (not KWType::IsSimple(attribute->GetType()))
			{
				bOk = false;
				AddError("Treatement variable " + sTreatementAttributeName + " of type " +
					 KWType::ToString(attribute->GetType()));
			}
			else if (not attribute->GetUsed())
			{
				bOk = false;
				AddError("Treatement variable " + sTreatementAttributeName + " unused in dictionary " +
					 kwcClass->GetName());
			}
		}
	}

	// Verification des algorithmes de pretraitement selon le type de la cible
	if (not preprocessingSpec.CheckForTargetType(GetTreatementAttributeType()))
	{
		bOk = false;
		AddError("Incorrect specification for preprocessing algorithms");
	}

	// Verification de la synchronisation du type de l'attribut cible
	ensure(not bOk or (sTreatementAttributeName == "" and nTreatementAttributeType == KWType::None) or
	       (not bCheckTreatementAttribute or
		kwcClass->LookupAttribute(sTreatementAttributeName)->GetType() == nTreatementAttributeType));
	ensure(nTreatementAttributeType == KWType::Unknown or
	       (nTreatementAttributeType == KWType::None and treatementDescriptiveStats == NULL) or
	       (nTreatementAttributeType == KWType::Continuous and
		cast(KWDescriptiveContinuousStats*, treatementDescriptiveStats) != NULL) or
	       (nTreatementAttributeType == KWType::Symbol and
		cast(KWDescriptiveSymbolStats*, treatementDescriptiveStats) != NULL));
	ensure(nTreatementAttributeType == KWType::Unknown or
	       (nTreatementAttributeType == KWType::None and treatementValueStats == NULL) or
	       treatementValueStats != NULL);
	ensure(nTreatementAttributeType == KWType::Unknown or treatementDescriptiveStats == NULL or
	       (treatementDescriptiveStats->GetLearningSpec() == this and
		treatementDescriptiveStats->GetAttributeName() == sTreatementAttributeName));
	return bOk;
}

void UPLearningSpec::SetCheckTreatementAttribute(boolean bValue)
{
	bCheckTreatementAttribute = bValue;
}

boolean UPLearningSpec::GetCheckTreatementAttribute() const
{
	return bCheckTreatementAttribute;
}

const ALString UPLearningSpec::GetClassLabel() const
{
	return "Uplift Learning specification";
}

const ALString UPLearningSpec::GetObjectLabel() const
{
	ALString sLabel;

	// Nom de la classe
	if (GetClass() == NULL)
		sLabel = "???";
	else
		sLabel = GetClass()->GetName();

	// Nom de l'attribut
	sLabel += " ";
	if (GetTreatementAttributeName() != "")
		sLabel += GetTreatementAttributeName();

	return sLabel;
}

void UPLearningSpec::ResetTreatementStats()
{
	// Initialisation du nombre d'instances
	nInstanceNumber = 0;

	// Synchronisation avec les statistique descriptives de l'attribut cible
	if (treatementDescriptiveStats != NULL)
		delete treatementDescriptiveStats;
	treatementDescriptiveStats = NULL;
	if (nTreatementAttributeType == KWType::Continuous)
		treatementDescriptiveStats = new KWDescriptiveContinuousStats;
	else if (nTreatementAttributeType == KWType::Symbol)
		treatementDescriptiveStats = new KWDescriptiveSymbolStats;
	if (treatementDescriptiveStats != NULL)
	{
		treatementDescriptiveStats->SetLearningSpec(this);
		treatementDescriptiveStats->SetAttributeName(sTreatementAttributeName);
	}

	// Synchronisation avec les statistiques par valeurs de l'attribut cible
	if (treatementValueStats != NULL)
		delete treatementValueStats;
	treatementValueStats = NULL;
	if (nTreatementAttributeType == KWType::Continuous or nTreatementAttributeType == KWType::Symbol)
		treatementValueStats = new KWDataGridStats;

	// Index de la modalite cible principale
	nMainTreatementModalityIndex = -1;

	// Mise a false du flag de calcul des stats
	bIsTreatementStatsComputed = false;
}

int UPLearningSpec::ComputeMainTreatementModalityIndex() const
{
	int nIndex;
	const KWDGSAttributeSymbolValues* symbolValues;

	nIndex = -1;
	if (GetTreatementAttributeType() == KWType::Symbol and treatementValueStats != NULL and
	    treatementValueStats->GetAttributeNumber() > 0 and not sMainTreatementModality.IsEmpty())
	{
		assert(treatementValueStats->GetAttributeNumber() == 1);
		symbolValues = cast(const KWDGSAttributeSymbolValues*, treatementValueStats->GetAttributeAt(0));
		nIndex = symbolValues->ComputeSymbolPartIndex(sMainTreatementModality);
	}
	return nIndex;
}

void UPLearningSpec::ComputeNullCost()
{
	KWDiscretizerMODL discretizerMODL;
	KWDiscretizerMODLFamily* discretizerMODLFamily;
	KWDataGrid nullDataGrid;
	ObjectArray oaParts;
	KWDataGridRegressionCosts nullRegressionCost;
	KWFrequencyTable nullFrequencyTable;
	KWFrequencyTable* kwftNullPreparedTable;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int i;
	int nValueNumber;

	require(GetTargetAttributeName() == "" or GetTargetValueStats() != NULL);

	// Cas Symbol
	if (GetTargetAttributeType() == KWType::Symbol)
	{
		// Recherche du discretiseur MODL, qui peut avoir ete redefini (cas des arbres de decision par exemple)
		if (GetPreprocessingSpec()
			->GetDiscretizerSpec()
			->GetDiscretizer(GetTargetAttributeType())
			->IsMODLFamily())
			//discretizerMODLFamily = cast(
			//   KWDiscretizerMODLFamily*,
			//  GetPreprocessingSpec()->GetDiscretizerSpec()->GetDiscretizer(GetTargetAttributeType()));
			discretizerMODLFamily = &discretizerMODL;
		// Sinon, on prend le discretiseur MODL standard
		else
			discretizerMODLFamily = &discretizerMODL;

		// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
		nTargetModalityNumber = GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber();
		nValueNumber = nTargetModalityNumber;
		nTreatementModalityNumber = GetTreatementValueStats()->GetAttributeAt(0)->GetPartNumber();
		//nullFrequencyTable.SetFrequencyVectorCreator(new UPDenseFrequencyVector);
		//InitFrequencyVector(nullFrequencyTable.GetFrequencyVectorCreator());
		nullFrequencyTable.SetFrequencyVectorNumber(1);

		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, nullFrequencyTable.GetFrequencyVectorAt(0));
		//int test = kwdfvFrequencyVector->GetTargetModalityNumber();
		//test = kwdfvFrequencyVector->GetTreatementModalityNumber();
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(nValueNumber);
		ivFrequencyVector->Initialize();
		// Alimentation de cette ligne par les frequences globales des valeurs cibles
		assert(GetTargetDescriptiveStats()->GetValueNumber() == GetTargetValueStats()->ComputeTargetGridSize());
		for (i = 0; i < nValueNumber; i++)
		{
			ivFrequencyVector->SetAt(i, GetTargetValueStats()->GetUnivariateCellFrequencyAt(i));
		}

		nullFrequencyTable.SetInitialValueNumber(nullFrequencyTable.GetTotalFrequency());
		nullFrequencyTable.SetGranularizedValueNumber(nullFrequencyTable.GetInitialValueNumber());

		// Utilisation du discretiseur specifie dans les pretraitements
		discretizerMODLFamily->Discretize(&nullFrequencyTable, kwftNullPreparedTable);

		// Memorisation des couts MODL
		dNullConstructionCost =
		    discretizerMODLFamily->ComputeDiscretizationConstructionCost(&nullFrequencyTable);
		dNullPreparationCost = discretizerMODLFamily->ComputeDiscretizationPreparationCost(&nullFrequencyTable);
		dNullDataCost = discretizerMODLFamily->ComputeDiscretizationDataCost(&nullFrequencyTable);

		// Nettoyage
		delete kwftNullPreparedTable;
	}
	// Cas non supervise
	else
	{
		// Cout pour le choix du modele null
		dNullConstructionCost = log(2.0);

		// Ces couts dependent du nombre et du type de variables (numerique ou categoriel)
		dNullPreparationCost = 0;
		dNullDataCost = 0;
	}
}

void UPLearningSpec::InitFrequencyTable(KWFrequencyTable* kwftSource)
{
	int i;
	kwftSource->SetFrequencyVectorCreator(new UPDenseFrequencyVector);
	InitFrequencyVector(kwftSource->GetFrequencyVectorCreator());
	for (i = 0; i < kwftSource->GetFrequencyVectorNumber(); i++)
	{
		InitFrequencyVector(kwftSource->GetFrequencyVectorAt(i));
	}
}
void UPLearningSpec::InitFrequencyVector(const KWFrequencyVector* kwfvVector)
{
	require(kwfvVector->GetClassLabel() == "Uplift Dense frequency vector");
	cast(UPDenseFrequencyVector*, kwfvVector)->SetTargetModalityNumber(nTargetModalityNumber);
	cast(UPDenseFrequencyVector*, kwfvVector)->SetTreatementModalityNumber(nTreatementModalityNumber);
}

boolean UPLearningSpec::CheckFrequencyTable(KWFrequencyTable* kwftSource)
{
	int i;

	if (kwftSource->GetFrequencyVectorCreator()->GetClassLabel() != "Uplift Dense frequency vector")
		return false;

	for (i = 0; i < kwftSource->GetFrequencyVectorNumber(); i++)
	{
		if (CheckFrequencyVector(kwftSource->GetFrequencyVectorAt(i)) == false)
			return false;
	}
	return true;
}
boolean UPLearningSpec::CheckFrequencyVector(const KWFrequencyVector* kwfvVector)
{
	if (kwfvVector->GetClassLabel() != "Uplift Dense frequency vector")
		return false;
	if (cast(UPDenseFrequencyVector*, kwfvVector)->GetTargetModalityNumber() != nTargetModalityNumber)
		return false;
	if (cast(UPDenseFrequencyVector*, kwfvVector)->GetTreatementModalityNumber() != nTreatementModalityNumber)
		return false;
	return true;
}
