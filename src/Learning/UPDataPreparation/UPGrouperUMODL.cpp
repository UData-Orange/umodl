// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPGrouperUMODL.h"
#include "UPUnivariatePartitionCost.h"

UPGrouperUMODL::UPGrouperUMODL()
{
	delete groupingCosts;
	groupingCosts = new UPMODLGroupingCosts;
}

const ALString UPGrouperUMODL::GetName() const
{
	return "UMDOL";
}

KWGrouper* UPGrouperUMODL::Create() const
{
	return new UPGrouperUMODL;
}