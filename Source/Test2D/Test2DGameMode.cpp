// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Test2DGameMode.h"
#include "Test2DCharacter.h"

ATest2DGameMode::ATest2DGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = ATest2DCharacter::StaticClass();	
}
