// Copyright 2020 Ryan Gourley


#include "NonPlatformMaster.h"

// Sets default values
ANonPlatformMaster::ANonPlatformMaster()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

};

// Called when the game starts or when spawned
void ANonPlatformMaster::BeginPlay()
{
	Super::BeginPlay();

};

// Called every frame
void ANonPlatformMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

};

