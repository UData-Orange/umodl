// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataPreparationClass.h"

#include "Object.h"

///////////////////////////////////////////////////////////////////////////////
// Classe de gestion de la preparation des donnees pour cas d'usage uplift
// Construction d'un dictionnaire de recodage des attributs a partir des
// statistiques descriptives
// Gestion des attributs et de leur recodage, en mode supervise ou non
class UPDataPreparationClass : public KWDataPreparationClass
{
public:
	void ComputeDataPreparationFromAttribStats(ObjectArray* oaAttribStats);
};
