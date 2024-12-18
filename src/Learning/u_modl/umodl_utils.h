// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "umodlCommandLine.h"

#include "KWAttribute.h"
#include "KWAttributeStats.h"
#include "KWClass.h"
#include "KWClassDomain.h"
#include "KWContinuous.h"
#include "KWDerivationRule.h"
#include "KWDiscretizer.h"
#include "KWFrequencyVector.h"
#include "KWGrouper.h"
#include "KWLearningSpec.h"
#include "KWSTDatabaseTextFile.h"
#include "KWSymbol.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "Object.h"
#include "PLParallelTask.h"
#include "Standard.h"
#include "UPLearningSpec.h"
#include "Vector.h"

// Objet de nettoyage de la memoire allouee au fil du programme umodl
class Cleaner
{
public:
	KWSTDatabaseTextFile* m_readDatabase = nullptr;       // la base de donnees a analyser
	ObjectArray* m_analysableAttributeStatsArr = nullptr; // les UPAttributeStats crees par l'analyse

	// action de la classe
	void operator()()
	{
		if (m_readDatabase)
		{
			m_readDatabase->GetObjects()->DeleteAll();
		}

		if (m_analysableAttributeStatsArr)
		{
			m_analysableAttributeStatsArr->DeleteAll();
		}

		// nettoyage des domaines
		KWClassDomain::GetCurrentDomain()->DeleteAllDomains();
		// Nettoyage des taches
		PLParallelTask::DeleteAllTasks();
		// Nettoyage des methodes de pretraitement
		KWDiscretizer::DeleteAllDiscretizers();
		KWGrouper::DeleteAllGroupers();
		// nettoyage des regles de derivation
		KWDerivationRule::DeleteAllDerivationRules();
	}
};

// // separation de tupletable en frequencytables selon les valeurs d'un attribut choisi
// void SeparateWRTSymbolAttribute(const KWTupleTable& inputTable, const int attribIdx, ObjectArray& outputArray)
// {
// 	require(attribIdx >= 0);

// 	// quelques constantes
// 	const int inputTableSize = inputTable.GetSize();
// 	const int inputTableAttribCount = inputTable.GetAttributeNumber();

// 	// suivi des valeurs de symbol vues pour l'attribut d'interet
// 	SymbolVector symbols;

// 	for (int i = 0; i < inputTableSize; i++)
// 	{
// 		const auto currentTuple = inputTable.GetAt(i);

// 		// examiner le symbol
// 		const auto& sym = currentTuple->GetSymbolAt(attribIdx);
// 		const int pos = SearchSymbol(symbols, sym);

// 		if (pos == symbols.GetSize())
// 		{
// 			// symbol pas encore vu
// 			symbols.Add(sym);

// 			// une nouvelle TupleTable doit etre creee
// 			auto newTable = new KWTupleTable;
// 			for (int att = 0; att < inputTableAttribCount; att++)
// 			{
// 				newTable->AddAttribute(inputTable.GetAttributeNameAt(att),
// 						       inputTable.GetAttributeTypeAt(att));
// 			}
// 			outputArray.Add(newTable);
// 		}

// 		// editer la TupleTable correspondante
// 		auto const outputTablePtr = cast(KWTupleTable*, outputArray.GetAt(pos));
// 		outputTablePtr->SetUpdateMode(true);
// 		auto const editTuple = outputTablePtr->GetInputTuple();
// 		for (int att = 0; att < inputTableAttribCount; att++)
// 		{
// 			switch (inputTable.GetAttributeTypeAt(att))
// 			{
// 			case KWType::Continuous:
// 				editTuple->SetContinuousAt(att, currentTuple->GetContinuousAt(att));
// 				break;

// 			case KWType::Symbol:
// 				editTuple->SetSymbolAt(att, currentTuple->GetSymbolAt(att));
// 				break;

// 			default:
// 				//TODO comment gerer ce cas ?
// 				break;
// 			}
// 		}
// 		editTuple->SetFrequency(currentTuple->GetFrequency());
// 		outputTablePtr->UpdateWithInputTuple();
// 		outputTablePtr->SetUpdateMode(false);
// 	}
// }

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

void InitAndComputeAttributeStats(KWAttributeStats& stats, const ALString& name, const int type,
				  KWLearningSpec& learningSpec, const KWTupleTable& table);

// void InitAndComputeStats(KWAttributeStats& attribStats, const KWAttribute& attrib, KWLearningSpec& learningSpec,
// 			 const KWTupleTable& tupleTable);

////////////////////////////////////////////////////////////////////////
bool CheckDictionnary(UMODLCommandLine& commandLine, const KWClass& dico, const ALString& attribTreatName,
		      const ALString& attribTargetName, ObjectArray& analysableAttribs);

bool CheckTreatmentAndTarget(UMODLCommandLine& commandLine, KWAttributeStats& treatStats,
			     const ALString& attribTreatName, KWAttributeStats& targetStats,
			     const ALString& attribTargetName, KWLearningSpec& learningSpec, KWTupleTableLoader& loader,
			     KWTupleTable& univariate);

bool CheckAnalysableAttributes(UMODLCommandLine& commandLine, const ObjectArray& analysableAttribsInput,
			       ObjectArray& analysableAttributeStatsArrOutput, KWLearningSpec& learningSpec,
			       KWTupleTableLoader& loader, KWTupleTable& univariate);

enum class StatPreparationMode
{
	Unsupervised,
	Supervised
};

bool PrepareStats(const ALString& attributeName, KWLearningSpec& learningSpec, KWTupleTableLoader& loader,
		  KWTupleTable& univariate, const StatPreparationMode mode);

////////////////////////////////////////////////////////////////////////
void WriteJSONReport(const ALString& sJSONReportName, const UPLearningSpec& learningSpec, ObjectArray& attribStats);

////////////////////////////////////////////////////////////////////////
void RegisterDiscretizers();

void RegisterParallelTasks();
