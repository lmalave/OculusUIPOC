// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusUIPOC.h"
#include "OculusUIPOCGameMode.h"
#include "OculusUIPOCHUD.h"
#include "OculusUIPOCCharacter.h"
#include "OculusUIPOCPlayerController.h"

AOculusUIPOCGameMode::AOculusUIPOCGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/FirstPersonCharacter"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;
	DefaultPawnClass = AOculusUIPOCCharacter::StaticClass();
	PlayerControllerClass = AOculusUIPOCPlayerController::StaticClass();

	// use our custom HUD class
	HUDClass = AOculusUIPOCHUD::StaticClass();
}
