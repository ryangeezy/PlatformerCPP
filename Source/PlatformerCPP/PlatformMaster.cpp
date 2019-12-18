// Copyright Ryan Gourley 2019

#include "PlatformMaster.h"
#include "DimenseCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"

// Sets default values
APlatformMaster::APlatformMaster()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TopTraceTag = FName("TopTraceTag");
	TopQParams = FCollisionQueryParams(TopTraceTag, false, this);
}

// Called when the game starts or when spawned
void APlatformMaster::BeginPlay()
{
	Super::BeginPlay();
	PlayerReference = Cast<ADimenseCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (PlayerReference->bDebugPlatformRemote){
		//GetWorld()->DebugDrawTraceTag = TopTraceTag;
	}
}

// Called every frame
void APlatformMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Checks if there is another platform above this platform by tracing a box the width and height of the player from where the player will land
bool APlatformMaster::PlatformAbovePlatformCheck(FVector FootLocation) 
{
	FVector Offset = PlayerReference->GetActorLocation() + PlayerReference->GetDimenseOffset(this, false);
	FVector Start = FVector(Offset.X, Offset.Y, FootLocation.Z);
	FVector End = Start + FVector(0, 0, PlayerReference->MyHeight);
	FCollisionShape NewBox; NewBox.SetBox(FVector(PlayerReference->MyWidth, PlayerReference->MyWidth, 0)); //20,20 = width of the player
	GetWorld()->SweepSingleByChannel(TopHitResult, Start, End, FRotator(0,0,0).Quaternion(), ECollisionChannel::ECC_WorldStatic, NewBox, TopQParams, TopRParams);
	APlatformMaster* HitObject = Cast<APlatformMaster>(TopHitResult.GetActor());
	if (HitObject){ //&& HitObject->GetFullName() != this->GetFullName()) {
		if (PlayerReference->bDebugPlatformRemote){
			FVector Origin; FVector Extent; GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, FColor::Turquoise, false, .5, 95, 10);
		}
		return true;
	}
	return false;
}

