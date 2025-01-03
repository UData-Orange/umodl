// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPDataPreparationClass.h"

#include "KWAttribute.h"
#include "KWClass.h"
#include "Standard.h"

void UPDataPreparationClass::ComputeDataPreparationFromAttribStats(ObjectArray* oaAttribStats)
{
	const ALString& sLevelMetaDataKey = KWDataPreparationAttribute::GetLevelMetaDataKey();
	// Nettoyage des specifications de preparation
	DeleteDataPreparation();

	// Duplication de la classe
	const KWClass* const classPtr = GetClass();
	kwcdDataPreparationDomain = classPtr->GetDomain()->CloneFromClass(classPtr);
	kwcDataPreparationClass = kwcdDataPreparationDomain->LookupClass(classPtr->GetName());

	// Nettoyage des meta-donnees de Level
	kwcDataPreparationClass->RemoveAllAttributesMetaDataKey(KWDataPreparationAttribute::GetLevelMetaDataKey());

	// Preparation de l'attribut cible
	if (not GetTargetAttributeName().IsEmpty())
	{
		dataPreparationTargetAttribute = new KWDataPreparationTargetAttribute;
		check(dataPreparationTargetAttribute);
		dataPreparationTargetAttribute->InitFromAttributeValueStats(kwcDataPreparationClass,
									    GetTargetValueStats());
	}

	// Ajout d'attributs derives pour toute stats de preparation disponible (univarie, bivarie...)
	for (int i = 0; i < oaAttribStats->GetSize(); i++)
	{
		KWDataPreparationStats* const preparedStats = cast(KWDataPreparationStats*, oaAttribStats->GetAt(i));

		// Meta-donne de Level sur l'attribut natif, uniquement dans le cas univarie
		if (preparedStats->GetTargetAttributeType() != KWType::None and
		    preparedStats->GetAttributeNumber() == 1)
		{
			// Recherche de l'attribute dans la classe de preparation
			KWAttribute* const nativeAttribute =
			    kwcDataPreparationClass->LookupAttribute(preparedStats->GetAttributeNameAt(0));

			// Parametrage de l'indication de Level
			nativeAttribute->GetMetaData()->SetDoubleValueAt(sLevelMetaDataKey, preparedStats->GetLevel());
		}

		// Ajout d'un attribut derive s'il existe une grille de donnees
		if (preparedStats->GetPreparedDataGridStats())
		{
			// Memorisation des infos de preparation de l'attribut
			KWDataPreparationAttribute* const dataPreparationAttribute = new KWDataPreparationAttribute;
			check(dataPreparationAttribute);
			dataPreparationAttribute->InitFromDataPreparationStats(kwcDataPreparationClass, preparedStats);
			oaDataPreparationAttributes.Add(dataPreparationAttribute);
			dataPreparationAttribute->SetIndex(oaDataPreparationAttributes.GetSize() - 1);
		}
	}
}
