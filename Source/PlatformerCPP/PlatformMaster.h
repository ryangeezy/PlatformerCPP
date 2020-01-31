// Copyright Ryan Gourley 2019

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
	UPROPERTY(VisibleAnywhere, Category = "Platform", meta = (AllowPrivateAccess = "true", Tooltip = "Reference to the player as DimenseCharacter."))
		ADimenseCharacter* PlayerReference;

	UPROPERTY(VisibleAnywhere, Category = "Platform", meta = (AllowPrivateAccess = "true", Tooltip = "TraceTag for vertical box trace from top of this platform - PlatformAbovePlatformCheck()."))
		FName TopTraceTag;

	UPROPERTY(VisibleAnywhere, Category = "Platform", meta = (AllowPrivateAccess = "true", Tooltip = "HitResult for vertical box trace from top of this platform - PlatformAbovePlatformCheck()."))
		FHitResult TopHitResult;

	//UPROPERTY(VisibleAnywhere, Category = "Platform Variables", meta = (AllowPrivateAccess = "true"))
	FCollisionQueryParams TopQParams;

	//UPROPERTY(VisibleAnywhere, Category = "Platform Variables", meta = (AllowPrivateAccess = "true"))
	FCollisionResponseParams TopRParams;
};
