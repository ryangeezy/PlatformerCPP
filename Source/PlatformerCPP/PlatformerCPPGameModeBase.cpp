// Copyright Ryan Gourley 2019

#include "PlatformerCPPGameModeBase.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "DimenseCharacter.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"

void APlatformerCPPGameModeBase::Debug()
{
	PlayerReference = Cast<ADimenseCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
	//Toggle debug if the command is entered
	if (PlayerReference->bDebug) {
		PlayerReference->bDebug = false;
		PlayerReference->bDebugPlatformRemote = false;
	}
	else {
		PlayerReference->bDebug = true;
		PlayerReference->bDebugPlatformRemote = true;
	}
}