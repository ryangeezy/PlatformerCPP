// Copyright 2020 Ryan Gourley

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformMaster.generated.h"

class UCapsuleComponent;
class ADimenseCharacter;

UCLASS()
class PLATFORMERCPP_API APlatformMaster : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	APlatformMaster();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Platform", meta = (Tooltip = "Checks if there is another platform above this platform by tracing a box the width and height of the player from where the player will land."))
		bool PlatformAbovePlatformCheck();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	ADimenseCharacter* PlayerReference;
	FHitResult AboveHitResult;
	FName TraceTag;
	FCollisionQueryParams QParams;	
};
