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
}

// Called every frame
void APlatformMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Checks if there is another platform above this platform by tracing a box the width and height of the player from where the player will land
bool APlatformMaster::PlatformAbovePlatformCheck() 
{
	FVector Origin; FVector Extent; GetActorBounds(true, Origin, Extent);
	FVector Offset = PlayerReference->GetActorLocation() + PlayerReference->GetDimenseOffset(this, false);
	FVector Start = FVector(Offset.X, Offset.Y, this->GetActorLocation().Z+(Extent.Z/2));
	FVector End = Start + FVector(0, 0, PlayerReference->MyHeight);
	FVector PlayerWidthOffset = FVector(5, 5, 0);
	FCollisionShape NewBox; NewBox.SetBox(FVector(PlayerReference->MyWidth/2, PlayerReference->MyWidth/2, 0) - (PlayerWidthOffset/2)); //box trace, width of the player
	GetWorld()->SweepSingleByChannel(TopHitResult, Start, End, FRotator(0,0,0).Quaternion(), ECollisionChannel::ECC_WorldStatic, NewBox, TopQParams, TopRParams);
	APlatformMaster* HitObject = Cast<APlatformMaster>(TopHitResult.GetActor());
	if (HitObject){ //&& HitObject->GetFullName() != this->GetFullName()) {
		if (PlayerReference->bDebugPlatformRemote){
			DrawDebugBox(GetWorld(), Offset, FVector(PlayerReference->MyWidth/2, PlayerReference->MyWidth/2, PlayerReference->MyHeight/2) - (PlayerWidthOffset/2), FColor::Red, false, .5, 95, 5);
			DrawDebugBox(GetWorld(), Origin, Extent, FColor::Turquoise, false, .5, 95, 10);
		}
		return true;
	}
	return false;
}

