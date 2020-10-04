// Copyright 2020 Ryan Gourley

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class UCameraComponent;
class ADimenseCharacter;

UCLASS()
class PLATFORMERCPP_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Pickup Functions", meta = (AllowPrivateAccess = "true", Tooltip = "Called when an object is hit by the player pickup trace to check if there are any objects in the way."))
	bool AttemptTraceBackToPlayer();

	UFUNCTION(Category = "Pickup Functions", meta = (AllowPrivateAccess = "true", BlueprintInternalUseOnly = "true", Tooltip = "Called by AttemptTraceBackToPlayer() to do the trace for the check of objects in the way."))
	bool BoxTraceForPickupObstacles(FHitResult &HitResult);

	UPROPERTY(VisibleAnywhere, Category = "Pickup Variables", meta = (AllowPrivateAccess = "true", Tooltip = "Reference to the player as DimenseCharacter."))
	ADimenseCharacter* PlayerReference;

	UPROPERTY(VisibleAnywhere, Category = "Pickup Variables", meta = (AllowPrivateAccess = "true", Tooltip = "HitResult returned by AttemptTraceBackToPlayer(). If the object hit is the player, the pickup will be picked up."))
	FHitResult PickupHitResult;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Variables", meta = (AllowPrivateAccess = "true", Tooltip = "Object types that can block the player from picking up an object (walls, etc.)."))
	TArray<TEnumAsByte<EObjectTypeQuery>> PickupBlockerObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Variables", meta = (AllowPrivateAccess = "true", Tooltip = "Actors to ignore during the trace back to the player from the pickup."))
	TArray<AActor*> PickupBlockerIgnoreActors;
};
