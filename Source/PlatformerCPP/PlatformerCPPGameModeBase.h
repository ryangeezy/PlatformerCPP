// Copyright 2020 Ryan Gourley

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlatformerCPPGameModeBase.generated.h"

class ADimenseCharacter;

/**
 * 
 */
UCLASS()
class PLATFORMERCPP_API APlatformerCPPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(Exec, Category = "Debug")
	void Debug();

	UPROPERTY(VisibleAnywhere, Category = "Pickup Variables", meta = (AllowPrivateAccess = "true", Tooltip = "Reference to the player as DimenseCharacter."))
	ADimenseCharacter* PlayerReference;
};
