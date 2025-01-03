// Copyright (c) 2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "UPSTDatabaseStream.h"

#include "KWDataTableDriverStream.h"

UPSTDatabaseStream::UPSTDatabaseStream()
{
	// Parametrage du driver de table en remplacant celui de la classe ancetre
	assert(dataTableDriverCreator != NULL);
	delete dataTableDriverCreator;
	dataTableDriverCreator = new KWDataTableDriverStream;
}

ALString UPSTDatabaseStream::GetTechnologyName() const
{
	return "Single table stream";
}

void UPSTDatabaseStream::SetHeaderLine(const ALString& sValue)
{
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetHeaderLine(sValue);
}

const ALString& UPSTDatabaseStream::GetHeaderLine() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetHeaderLine();
}

void UPSTDatabaseStream::SetFieldSeparator(char cValue)
{
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetFieldSeparator(cValue);
}

char UPSTDatabaseStream::GetFieldSeparator() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetFieldSeparator();
}

KWObject* UPSTDatabaseStream::ReadFromBuffer(const char* sBuffer)
{
	require(sBuffer != NULL);

	KWDataTableDriverStream* dataTableDriverStream = cast(KWDataTableDriverStream*, dataTableDriverCreator);

	// Alimentation du buffer en entree
	dataTableDriverStream->FillBufferWithRecord(sBuffer);

	// Lecture de l'objet
	KWObject* kwoObject = Read();

	// ajout de l'objet dans le tableau des objets
	oaAllObjects.Add(kwoObject);

	return kwoObject;
}

boolean UPSTDatabaseStream::WriteToBuffer(KWObject* kwoObject, char* sBuffer, int nMaxBufferLength)
{
	// Vidage du buffer en sortie
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->ResetOutputBuffer();

	// Ecriture de l'objet dans le buffer du stream
	Write(kwoObject);

	// On recupere le resultat
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->FillRecordWithBuffer(sBuffer, nMaxBufferLength);
}

void UPSTDatabaseStream::SetMaxBufferSize(int nValue)
{
	require(nValue > 0);
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetBufferSize(nValue);
}

int UPSTDatabaseStream::GetMaxBufferSize() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetBufferSize();
}
