// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SungminWorldGameMode.h"
#include "SungminWorldCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASungminWorldGameMode::ASungminWorldGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_Player"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}	
}
