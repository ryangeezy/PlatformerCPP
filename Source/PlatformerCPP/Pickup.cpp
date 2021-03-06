// Copyright 2020 Ryan Gourley

#include "Pickup.h"
#include "WorldCollision.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraTypes.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "DimenseCharacter.h"
#include "DimensePlayerController.h"

// Sets default values
APickup::APickup(){
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	PickupBlockerObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	PickupBlockerObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	PickupBlockerObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	PickupBlockerIgnoreActors.Add(this);
}

// Called when the game starts or when spawned
void APickup::BeginPlay(){
	Super::BeginPlay();

	PlayerReference = Cast<ADimenseCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
}

// Called every frame
void APickup::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
}

//Called when an object is hit by the player pickup trace to check if there are any objects in the way
bool APickup::AttemptTraceBackToPlayer(){
	if (BoxTraceForPickupObstacles(PickupHitResult)) {
		if (Cast<ADimenseCharacter>(PickupHitResult.GetActor())) {
			return true;
		}
	}
	return false;
}

//Called by AttemptTraceBackToPlayer() to do the trace for the check of objects in the way
bool APickup::BoxTraceForPickupObstacles(FHitResult &HitResult){
	FVector Origin;	FVector Extent;	GetActorBounds(true, Origin, Extent);
	if (Extent.X > Extent.Y) {
		Extent.Y = Extent.X;
	}else {
		Extent.X = Extent.Y;
	}
	float Distance = GetHorizontalDistanceTo(PlayerReference);
	int32 Direction;
	if ((PlayerReference->GetActorLocation() - PlayerReference->MainCamera->GetComponentLocation()).Size() > (GetActorLocation() - PlayerReference->MainCamera->GetComponentLocation()).Size()) {
		Direction = -1;
	}else{
		Direction = 1;
	}
	
	FVector LineVector = PlayerReference->CamForwardVector * Distance * Direction;
	FVector End = Origin - LineVector;
	return UKismetSystemLibrary::BoxTraceSingleForObjects(GetWorld(), Origin, End, Extent, FRotator(0), PickupBlockerObjectTypes, false, PickupBlockerIgnoreActors, EDrawDebugTrace::None, HitResult, true, FLinearColor::Red, FLinearColor::Green, .01);
}