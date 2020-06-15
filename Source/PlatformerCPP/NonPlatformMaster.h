// Copyright 2020 Ryan Gourley

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NonPlatformMaster.generated.h"

UCLASS()
class PLATFORMERCPP_API ANonPlatformMaster : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANonPlatformMaster();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
