// Copyright 2020 Ryan Gourley

#include "DimenseCharacter.h"
#include "Runtime/Engine/Public/LatentActions.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraTypes.h"
#include "Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h"
#include "Runtime/Engine/Classes/GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Components/BoxComponent.h"
#include "Runtime/Engine/Classes/Components/CapsuleComponent.h"
#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "Runtime/Engine/Public/WorldCollision.h"
#include "DrawDebugHelpers.h"
#include "DimensePlayerController.h"
#include "PlatformMaster.h"
#include "SurfacePlatformComponent.h"

// Sets default values
ADimenseCharacter::ADimenseCharacter(){
	//Unreal Variables
	PrimaryActorTick.bCanEverTick = true;

	//My Variables
	bDebug = true; //Is debug print, views, control panel, etc. enabled?
	bDebugVisibility = false;
	bDebugGround = true;
	bDebugTransport = true;
	bDebugAbovePlatform = true;
	bDebugMoveAround = true;
	bDebugInvalidation = true;
	bDebugCachedInvalidation = false;
	bDebugPlatformRemote = true; //Is debug drawing for platform checks by platforms?
	bCanMoveAround = true; //Can the movement system move the character around walls?
	bCanTransport = true; //Can the movement system move the character to platforms "based on a 2 dimensional view"?
	bSpinning = false; //Is the camera actively spinning to a new 90 degree view?
	PhysicsComp = GetCapsuleComponent(); //Set the physics component
	MyHeight = 100.0f;//PhysicsComp->GetScaledCapsuleHalfHeight() * 2; //Player Height
	MyWidth = 50.0f;//PhysicsComp->GetScaledCapsuleRadius() * 2; //Player Width
	LandingOffsetPadding = FVector(MyWidth, MyWidth, 0.0f); //Distance used to correct player position when moved by the movement system
	DefaultSpringArmLength = 2097152.0f; //Distance from camera to the player
	SideTraceLength = 25.0f; //Length of lines drawn to detect walls/edges to move around
	WalkAcceleration = 16.0f; //Acceleration speed while walking
	HeadTraceLength = 10.0f; //Length of the line used to detect the ground
	GroundTraceLength = 5.0f; //Length of the line used to detect the ground
	DeathDistance = 2500.0f; //Distance you can fall before being reset
	MeshRotationTime = 0.05f; //Time it takes for the mesh to correct rotation for a new 90 degree view
	CameraZoomTime = 2.0f; //Time it takes for the camera to zoom into position when a map is loaded
	TransportTraceZOffset = 25.0f; //Offset Z for Transport line trace
	NullVector = FVector(0.0f, 0.0f, 0.0f);
	HeightPadding = FVector(0.0f, 0.0f, 5.0f);

	//Debug Trace Tagging
	TraceTag = FName(TEXT("TraceTag"));
	QParams = FCollisionQueryParams(TraceTag, false, this);

	//SubObjects------------------------------------------------------------------------------------------------------------------>>>

	//Main Spring Arm for the whole camera chassis.
	RotationSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("RotationSpringArm"));
	RotationSpringArm->SetupAttachment(RootComponent, FName(TEXT("RotationSpringArm")));
	RotationSpringArm->bDoCollisionTest = false;
	RotationSpringArm->bEnableCameraLag = false;
	RotationSpringArm->TargetArmLength = 0.0f;
	//Spring Arm for the main camera
	MainCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MainCameraSpringArm"));
	MainCameraSpringArm->SetupAttachment(RotationSpringArm, FName(TEXT("MainCameraSpringArm")));
	MainCameraSpringArm->bDoCollisionTest = false;
	MainCameraSpringArm->bEnableCameraLag = true;
	MainCameraSpringArm->TargetArmLength = DefaultSpringArmLength;
	//Main Camera
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(MainCameraSpringArm, FName(TEXT("MainCamera")));
	MainCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	MainCamera->FieldOfView = 0.1f;
	
	//AntiCamera is used as a point of reference for the location opposite of the camera (where the MainCamera would be if the Spring Arm was rotated 180 degrees)
	AntiCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("AntiCameraSpringArm"));
	AntiCameraSpringArm->SetupAttachment(RotationSpringArm, FName(TEXT("AntiCameraSpringArm")));
	AntiCameraSpringArm->bDoCollisionTest = false;
	AntiCameraSpringArm->bEnableCameraLag = true;
	AntiCameraSpringArm->TargetArmLength = MainCameraSpringArm->TargetArmLength * -1.0f;
	//Scene component for point of reference
	AntiCameraSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AntiCameraSceneComponent"));
	AntiCameraSceneComponent->SetupAttachment(AntiCameraSpringArm, FName(TEXT("AntiCameraSceneComponent")));
	//Spring Arm for the isometric debug window
	
	PnPCaptureSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PnPCaptureSpringArm"));
	PnPCaptureSpringArm->SetupAttachment(RotationSpringArm, FName(TEXT("PnPCaptureSpringArm")));
	PnPCaptureSpringArm->SetRelativeRotation(FRotator(-30.0f, -45.0f, 0.0f));
	PnPCaptureSpringArm->bDoCollisionTest = false;
	PnPCaptureSpringArm->bEnableCameraLag = false;
	PnPCaptureSpringArm->TargetArmLength = 1048576.0f;
	
	//<<<------------------------------------------------------------------------------------------------------------------SubObjects

	//Set the length of the line used for platform testing to the distance of the camera from the player
	FromCameraLineLength = MainCameraSpringArm->TargetArmLength;
	//A vector based on FromCameraLineLength, typically multiplied by a CamForwardVector/CamRightVector (so either x or y becomes 0, based on 90 degree angles)
	FromCameraLineVector = FVector(FromCameraLineLength, FromCameraLineLength, 0.0f);
}

// Called when the game starts or when spawned
void ADimenseCharacter::BeginPlay(){
	Super::BeginPlay(); // DO NOT remove
	InitDebug();
}

// Called every frame
void ADimenseCharacter::Tick(float DeltaTime){
	Super::Tick(DeltaTime); // DO NOT remove

	//Keep track of the order of things here. The following order is most logical.
	//1: add movement from input (needs to happen before platform checks so they can happen in the same frame, in that order)
	//2: update variables (CamForwardVector, CamRightVector, CamSign/CamSide, Head/FootLocation --- These are used for the following calculations)
	//3: line traces and platform checks (check visibility, and for MoveAround and Transport cases)
	//4: set ground location if on platform (need the platform first)
	//5: rotate the character mesh to the player movement (this could change if platform checks moves the character, so it should be after)

	if (!bSpinning) {
		//Some variables need to be updated every frame, but only while not spinning/rotating camera because movement is paused
		AddMovementInput(GetMovementInputFRI()); //Player movement. This should remain at the beginning of tick
		UpdateMovementSystemVariables();
		DoLineTracesAndPlatformChecks(); //Line traces used by the movement system to determine when and where to move the player from/to platforms
	}
	if (GroundPlatform) { //If valid ground platform found
		FVector Origin; FVector Extent;
		GroundPlatform->GetActorBounds(true, Origin, Extent);
		float GroundZ = GroundPlatform->GetActorLocation().Z + Extent.Z;
		FVector PlayerLocation = GetActorLocation();
		GroundLocation = FVector(PlayerLocation.X, PlayerLocation.Y, GroundZ); //Set the ground location
	}
	if (GroundPlatform || FacingDirection != 0) {
		RotateMeshToMovement();
	}
	//Enable/Disable custom debug drawing/output (Platform outlines for example)
	if (bDebug) {
		Debug();
	}
}

void ADimenseCharacter::DoLineTracesAndPlatformChecks(){
	//Check if the player is on the ground...
	if (BoxTraceVertical(FootLocation, GroundHitResult, GroundTraceLength, GroundTraceLength, -1, TEXT("Ground"), 0, true)) { //if you are on the ground
		//if (PlayerAbovePlatformCheck(Cast<APlatformMaster>(GroundHitResult.GetActor()))) {
			SetPlatform(GroundPlatform, CachedGroundPlatform, GroundHitResult, FColor::FromHex(TEXT("240B00FF")), true); //brown
		//}
	}else{ //if you are not on the ground
		InvalidatePlatform(GroundPlatform, CachedGroundPlatform);
		//If you are falling downward, try to Transport
		if (IsFallingDownward()) {
			TryTransport();
		}else{
			//Check if there is something above and if so, move "around" it (forward or backward)
			if (VisibilitySide != 0) {
				if (BoxTraceVertical(HeadLocation, HeadHitResult, HeadTraceLength, 1, 1, TEXT("Head"), 1, true)) {
					TryMoveAround(HeadHitResult, FVector(0.0f, 0.0f, HeadTraceLength));
				}
			}
		}
	}	

	//Check if the player is moving to the right
	if (VisibilitySide != 0) {
		if (MovementDirection == 1) {
			//Check if there is something to the right and if so, move "around" it (forward or backward)
			if (RightHitCheck()) {
				TryMoveAround(RightHitResult, NullVector);
			}
		}
		else if (MovementDirection == -1) { //Check if the player is moving to the left
		   //Check if there is something to the left and if so, move "around" it (forward or backward)
			if (LeftHitCheck()) {
				TryMoveAround(LeftHitResult, NullVector);
			}
		}
	}
}

bool ADimenseCharacter::HorizontalHitCheck(const FVector& Location, FHitResult& HitResult){
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = GetActorLocation();
	float X = CamSide * (MyWidth / 2);
	float Y = CamSide * (MyWidth / 2);

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End)) {
		return true;
	}
	return false;
}

bool ADimenseCharacter::LeftHitCheck(){
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = GetActorLocation();
	float X = CamSide * (MyWidth / 2);
	float Y = CamSide * (MyWidth / 2);

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit0"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}

	Start = FootLocation;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit1"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit2"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}

	Start = Center;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit3"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit4"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}

	Start = HeadLocation;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit5"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	return false;
}

bool ADimenseCharacter::RightHitCheck(){
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = GetActorLocation();
	float X = CamSide * (MyWidth / 2);
	float Y = CamSide * (MyWidth / 2);

	Start = FootLocation;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit0"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit1"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	Start = Center;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit2"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	Start = Center;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit3"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	Start = HeadLocation;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit4"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit5"));
		if (bDebug && bDebugMoveAround) {
			DrawDebugLine(GetWorld(), Start, End, FColor::Orange, false, 1.0f, 0, 5.0f);
		}
		return true;
	}
	return false;
} 

bool ADimenseCharacter::TryTransport(){
	if (bCanTransport) {
		if (BoxTraceForTransportHit(TransportTraceZOffset)) {
			if (Cast<USurfacePlatformComponent>(TransportHitResult.GetActor()->FindComponentByClass(USurfacePlatformComponent::StaticClass()))) {
				if (SetPlatform(TransportPlatform, CachedTransportPlatform, TransportHitResult, FColor::Green, false)) {
					if (PlayerAbovePlatformCheck(TransportPlatform)) {
						if (!TransportPlatform->PlatformAbovePlatformCheck()) {
							Transport();
							StartCanTransportTimer();
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool ADimenseCharacter::BoxTraceForTransportHit(const float& ZOffset){
	FVector BoxSize = FVector((MyWidth / 2), (MyWidth / 2), 0);
	FCollisionShape Box = FCollisionShape::MakeBox(BoxSize);
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide;
	FVector FootZ = FootLocation - FVector(0, 0, ZOffset);
	FVector Start = FootZ - LineVector;
	FVector End = FootZ + LineVector;
	if (bDebug && bDebugTransport) {
		FVector Length = Start + End;
		FVector Center = Length / 2;
		FVector Extent = BoxSize + LineVector * 2;
		DrawDebugBox(GetWorld(), Center, Extent, FColor::Green, false, 0.01f, 0, 3.0f);
	}
	return GetWorld()->SweepSingleByChannel(TransportHitResult, Start, End, MainCamera->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams);
}

void ADimenseCharacter::Transport(){
	if (bDebug && bDebugTransport) {
		GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Green, (TEXT("Transport")));
	}
	PhysicsComp->AddWorldOffset(GetTransportOffset(TransportPlatform));
	StartCanMoveAroundTimer();
}

FVector ADimenseCharacter::GetTransportOffset(const APlatformMaster* Platform) const{
	FVector Origin; FVector Extent; Platform->GetActorBounds(true, Origin, Extent);
	FVector Offset = FVector(TransportHitResult.Location.X, TransportHitResult.Location.Y, Origin.Z + Extent.Z) - GetActorLocation();
	FVector TransportOffset = CamForwardVector.GetAbs() * (Offset + LandingOffsetPadding * CamSide * CamSign * VisibilitySide);
	if (bDebug && bDebugTransport) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Green, false, 0.5f, 0,10.0f);
	}
	return TransportOffset;
}

bool ADimenseCharacter::TryMoveAround(UPARAM(ref) FHitResult& HitResult, FVector BoxTraceOffset){
	if (bCanMoveAround) {
		if (BoxTraceForMoveAroundHit(HitResult, BoxTraceOffset)) {
			if (SetPlatform(MoveAroundPlatform, CachedMoveAroundPlatform, MoveAroundHitResult, FColor::Orange, true)) {
				MoveAround();
				return true;
			}
		}
	}
	return false;
}

bool ADimenseCharacter::BoxTraceForMoveAroundHit(FHitResult& HitResult, FVector Offset){
	FVector BoxSize = FVector(MyWidth / 2, MyWidth / 2, MyHeight / 2);
	FCollisionShape Box = FCollisionShape::MakeBox(BoxSize);
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide;
	FVector Start = GetActorLocation() - LineVector + Offset;
	FVector End = GetActorLocation() + LineVector + Offset;
	if (bDebug && bDebugMoveAround) {
		FVector Length = Start + End;
		FVector SweepCenter = Length / 2;
		FVector SweepExtent = BoxSize + LineVector / 2;
		DrawDebugBox(GetWorld(), SweepCenter, SweepExtent, FColor::Orange, false, 0.5f, 0, 3.0f);
	}
	return GetWorld()->SweepSingleByChannel(MoveAroundHitResult, Start, End, MainCamera->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams);
}

void ADimenseCharacter::MoveAround(){
	if (bDebug && bDebugMoveAround) {
		GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Orange, (TEXT("MoveAround")));
	}
	PhysicsComp->AddWorldOffset(GetMoveAroundOffset(MoveAroundHitResult.Location));
	StartCanTransportTimer();
}

FVector ADimenseCharacter::GetMoveAroundOffset(const FVector& Location) const{
	FVector Offset = Location - GetActorLocation();
	FVector MoveAroundOffset = CamForwardVector.GetAbs() * (Offset - LandingOffsetPadding * CamSide * CamSign * VisibilitySide);
	if (bDebug && bDebugMoveAround) {
		FVector Origin; FVector Extent; MoveAroundPlatform->GetActorBounds(true, Origin, Extent);
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation() - FVector(0.0f, 0.0f, MyHeight / 2), GetActorLocation() - FVector(0.0f, 0.0f, MyHeight / 2) + MoveAroundOffset, 500.0f, FColor::Orange, false, 5.0f, 54, 3.0f);
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Orange, false, 0.5f, 0, 10.0f);
	}
	return MoveAroundOffset;
}

bool ADimenseCharacter::BoxTraceHorizontal(FHitResult& HitResult, const float& TraceLength, const int32 UpOrDown, const FString DebugPhrase, const bool bDebugLocal){
	FVector BoxSize = FVector(CamSide * (MyWidth / 2), CamSide * (MyWidth / 2), MyHeight / 2);
	FCollisionShape Box = FCollisionShape::MakeBox(BoxSize);
	FVector Start = GetActorLocation();
	FVector End = Start + ((FVector(0.0f, 0.0f, TraceLength) * UpOrDown));
	if (GetWorld()->SweepSingleByChannel(HitResult, Start, End, PhysicsComp->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams)) {
		if (bDebug && bDebugLocal) {
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Black, (TEXT("%s was hit"), DebugPhrase));
			DrawDebugBox(GetWorld(), Start, Box.GetExtent(), FColor::Red, false, 0.1f, 0, 1.0f); //Brown
		}
		return true;
	}
	if (bDebug && bDebugLocal) {
		DrawDebugBox(GetWorld(), Start, Box.GetExtent(), FColor::Orange, false, 0.1f, 0, 1.0f);
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions below this line are considered complete and have no known problems /////////////////////////////////////////////////////////////////////////////////////////

bool ADimenseCharacter::BoxTraceVertical(const FVector& Location, FHitResult& HitResult, const float BoxHalfHeight, const float& TraceLength, const int32 UpOrDown, const FString DebugPhrase, const int32 DebugTime, const bool bDebugLocal){
	//UpOrDown should be 1 or -1
	FVector BoxSize = FVector(CamSide * (MyWidth / 3), CamSide * (MyWidth / 3), BoxHalfHeight);
	FCollisionShape Box = FCollisionShape::MakeBox(BoxSize);
	FVector End = Location + ((FVector(0.0f, 0.0f, TraceLength) * UpOrDown));
	if (GetWorld()->SweepSingleByChannel(HitResult, Location, End, PhysicsComp->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams)) {
		if (bDebug && bDebugLocal) {
			GEngine->AddOnScreenDebugMessage(-1, DebugTime, FColor::Black, (TEXT("%s"), DebugPhrase+FString(TEXT(" was hit"))));
			FVector DebugBoxOffset = FVector(0.0f, 0.0f, (End.Z - Location.Z) / 2);
			DrawDebugBox(GetWorld(), Location + DebugBoxOffset, Box.GetExtent() + DebugBoxOffset, FColor::Red, false, 0.1f, 0,1.0f); //Brown
		}
		return true;
	}
	if (bDebug && bDebugLocal) {
		FVector DebugBoxOffset = FVector(0.0f, 0.0f, (End.Z - Location.Z) / 2);
		DrawDebugBox(GetWorld(), Location + DebugBoxOffset, Box.GetExtent() + DebugBoxOffset, FColor::Green, false, 0.1f, 0,1.0f);
	}
	return false;
}

bool ADimenseCharacter::SetPlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, UPARAM(ref) FHitResult& HitResult, const FColor DebugColor, const bool bDebugLocal){
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(HitResult.GetActor());
	if (NewPlatform) {
		if (Platform != NewPlatform) {
			InvalidatePlatform(Platform, CachedPlatform);
			Platform = NewPlatform;
			if (bDebug && bDebugLocal) {
				FVector Origin; FVector Extent; Platform->GetActorBounds(true, Origin, Extent);
				DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, 0.5f, 255, 5.0f);
			}
			return true;
		}else{
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::PlayerAbovePlatformCheck(const APlatformMaster* Platform) const{
	if (bDebug && bDebugAbovePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::FromHex(TEXT("0081FFFF")), (TEXT("Player Above Platform Check"))); //Blue
	}
	FVector Origin;
	FVector Extent;
	Platform->GetActorBounds(true, Origin, Extent);
	FVector Top = Origin + FVector(0.0f, 0.0f, Extent.Z);
	if (Platform == GroundPlatform || FootLocation.Z >= Top.Z) {
		return true;
	}
	if (bDebug && bDebugAbovePlatform) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::FromHex(TEXT("0081FFFF")), false, 0.5f, 0, 10.0f); //Blue
	}
	return false;
}

void ADimenseCharacter::InvalidatePlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, const FColor DebugColor){
	if (Platform) {
		if (bDebug && bDebugInvalidation) {
			FVector Origin;	FVector Extent;	Platform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, 0.25f, 0,10.0f);
		}
		CachedPlatform = Platform;
		Platform = nullptr;
	}
}

void ADimenseCharacter::InvalidateCachedPlatform(UPARAM(ref) APlatformMaster*& CachedPlatform, const FColor DebugColor){
	if (CachedPlatform) {
		if (bDebug && bDebugCachedInvalidation) {
			FVector Origin;	FVector Extent;	CachedPlatform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, 0.25f, 0,10.0f);
		}
		CachedPlatform = nullptr;
	}
}

void ADimenseCharacter::UpdateMovementSystemVariables(){
	//The Cam* variables are used by the movement system to determine which vectors apply based on the camera angle, and also direction based on +/- values
	CamForwardVector = RoundVector(FVector(MainCamera->GetForwardVector()));
	CamRightVector = RoundVector(FVector(MainCamera->GetRightVector()));
	CamSign = FMath::Sign(CamRightVector.Y + CamRightVector.X);
	if (!FMath::IsNearlyZero(double(CamForwardVector.X), 0.1)) {
		CamSide = 1;
	}else{
		CamSide = -1;
	}

	//Head and foot location used by the movement system
	FootLocation = GetActorLocation() - (FVector(0.0f, 0.0f, MyHeight/2));
	HeadLocation = FootLocation + FVector(0.0f, 0.0f, MyHeight);
	SetMovementDirection();
	SetVisibilitySide();
}

void ADimenseCharacter::SetMovementDirection(){
	//Check if the player is moving to the right
	if (PhysicsComp->GetComponentVelocity().X * CamSign > 0 || PhysicsComp->GetComponentVelocity().Y * CamSign > 0) {
		MovementDirection = 1;
	}
	//Check if the player is moving to the left
	else if (PhysicsComp->GetComponentVelocity().X * CamSign < 0 || PhysicsComp->GetComponentVelocity().Y * CamSign < 0) {
		MovementDirection = -1;
	}
	//Check if the player is not moving
	else if (PhysicsComp->GetComponentVelocity().X == 0.0f && PhysicsComp->GetComponentVelocity().Y == 0.0f) {
		MovementDirection = 0;
	}
}

FVector ADimenseCharacter::GetMovementInputFRI(){ //(FRI = Frame Rate Independent)
	return GetWorld()->GetDeltaSeconds() * ConsumeMovementInputVector() * WalkAcceleration;
}

int32 ADimenseCharacter::MoveLeftRight(const float& AxisValue){
	GetCharacterMovement()->AddInputVector(CamRightVector * AxisValue);
	FacingDirection = FMath::Sign(AxisValue);
	return FacingDirection;
}

void ADimenseCharacter::JumpDown(){
	if (GroundPlatform) {
		if (BoxTraceForTransportHit(TransportTraceZOffset)) {
			if (SetPlatform(TransportPlatform, CachedTransportPlatform, TransportHitResult, FColor::Green, bDebugTransport)) {
				FVector Offset = FVector(1.0f, 1.0f, 0.0f) * (TransportHitResult.Location - GetActorLocation() - LandingOffsetPadding * CamForwardVector * VisibilitySide);
				PhysicsComp->AddWorldOffset(Offset);
				StartCanTransportTimer();
			}
		}
	}
}

bool ADimenseCharacter::IsFallingDownward() const{
	return PhysicsComp->GetComponentVelocity().Z < 0.0f;
}

bool ADimenseCharacter::CheckDeathByFallDistance(){
	if (DeathParticle) { //make sure the particle effect is selected in the blueprint
		if (GroundPlatform) {
			if (FootLocation.Z < GroundPlatform->GetActorLocation().Z - DeathDistance) {
				Die();
				return true;
			}
		}else if (CachedGroundPlatform) {
			if (FootLocation.Z < CachedGroundPlatform->GetActorLocation().Z - DeathDistance) {
				Die();
				return true;
			}
		}
	}
	return false;
}

void ADimenseCharacter::SetVisibilitySide(){
	//If something is in between the camera and player: 1 if player is visible, -1 if visible from the back, 0 if not visible from either side
	if (VisibilityCheck(MainCamera->GetComponentLocation())) { //visible from the front?
		VisibilitySide = 1;
	}else{
		VisibilitySide = -1;
		if (!VisibilityCheck(AntiCameraSceneComponent->GetComponentLocation())) { //visible from the back side?
			VisibilitySide = 0;
		}
	}
}

bool ADimenseCharacter::VisibilityCheck(const FVector& Start){
	int32 HitCount = 0;
	float Distance = MainCameraSpringArm->TargetArmLength;
	float X = CamSide * PhysicsComp->Bounds.BoxExtent.X;
	float Y = CamSide * PhysicsComp->Bounds.BoxExtent.Y;
	FVector PlayerLocation = GetActorLocation();
	FVector End;

	End = FootLocation;
	if (bDebug && bDebugVisibility) {
		DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.0f, 0, 5.0f);
	}
	if (SingleTrace(FrontHitResult, Start, End)) {
		HitCount++;
	}
	End = HeadLocation;
	if (bDebug && bDebugVisibility) {
		DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.0f, 0, 5.0f);
	}
	if (SingleTrace(FrontHitResult, Start, End)) {
		HitCount++;
	}
	End = PlayerLocation + FVector(X, Y, 0.0f);
	if (bDebug && bDebugVisibility) {
		DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.0f, 0, 5.0f);
	}
	if (SingleTrace(FrontHitResult, Start, End)) {
		HitCount++;
	}
	End = PlayerLocation - FVector(X, Y, 0.0f);
	if (bDebug && bDebugVisibility) {
		DrawDebugLine(GetWorld(), Start, End, FColor::White, false, 0.0f, 0, 5.0f);
	}
	if (HitCount > 2) {
		return false;
	}
	if (SingleTrace(FrontHitResult, Start, End)) {
		HitCount++;
	}
	if (HitCount > 2) {
		return false;
	}
	return true;
}

bool ADimenseCharacter::SingleTrace(UPARAM(ref) FHitResult& HitResult, const FVector& Start, const FVector& End) const{
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QParams);
	if (HitResult.IsValidBlockingHit()) {
		return true;
	}
	return false;
}

void ADimenseCharacter::PauseMovement(){
	DisableInput(GetWorld()->GetFirstPlayerController());
	bCanTransport = false;
	bCanMoveAround = false;
	GetCharacterMovement()->GravityScale = 0.0f;
	KillMomentum();
}

void ADimenseCharacter::KillMomentum(){
	CachedJumpKeyHoldTime = JumpKeyHoldTime;
	CachedComponentVelocity = PhysicsComp->GetPhysicsLinearVelocity();
	CachedCharacterMovementVelocity = GetCharacterMovement()->Velocity;
	PhysicsComp->SetAllPhysicsLinearVelocity(FVector(0.0f, 0.0f, 0.0f));
	GetCharacterMovement()->Velocity = FVector(0.0f, 0.0f, 0.0f);
}

void ADimenseCharacter::ResumeMovement(){
	GetCharacterMovement()->GravityScale = 1.0f;
	GetCharacterMovement()->Velocity = CachedCharacterMovementVelocity;
	PhysicsComp->SetAllPhysicsLinearVelocity(CachedComponentVelocity);
	JumpKeyHoldTime = CachedJumpKeyHoldTime;
	bSpinning = false;
	bCanMoveAround = true;
	bCanTransport = true;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

void ADimenseCharacter::RotateMeshToMovement(){
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(MainCamera->GetComponentLocation(), GetMesh()->GetComponentLocation());
	auto MeshLatentInfo = FLatentActionInfo();
	MeshLatentInfo.CallbackTarget = GetMesh();
	UKismetSystemLibrary::MoveComponentTo(GetMesh(), GetMesh()->GetRelativeLocation(), FRotator(0.0f, (LookAtRotation.Yaw) + 90.0f + -90.0f * FacingDirection, 0.0f), true, true, MeshRotationTime, false, EMoveComponentAction::Move, MeshLatentInfo);
}

void ADimenseCharacter::RotateCamera(const float& Rotation){
	if (!bSpinning) {
		for (int32 i = 1; i < 5; i++) {
			if (UKismetMathLibrary::EqualEqual_RotatorRotator(RotationSpringArm->GetRelativeRotation(), FRotator(0.0f, i * 90.0f, 0.0f), 0.001f)) {
				RotationSpringArmLatentInfo.ExecutionFunction = FName(TEXT("ResumeMovement"));
				RotationSpringArmLatentInfo.UUID = 123;
				RotationSpringArmLatentInfo.Linkage = 1;
				RotationSpringArmLatentInfo.CallbackTarget = this;
				UKismetSystemLibrary::MoveComponentTo(RotationSpringArm, RotationSpringArm->GetRelativeLocation(), FRotator(0.0f, i * 90.0f + Rotation, 0.0f), true, true, 0.6f, true, EMoveComponentAction::Move, RotationSpringArmLatentInfo);
				bSpinning = true;
				PauseMovement();
				InvalidatePlatform(GroundPlatform, CachedGroundPlatform);
				InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform);
				InvalidatePlatform(TransportPlatform, CachedTransportPlatform);
			}
		}
	}
}

void ADimenseCharacter::Die(){
	FTransform Spawn = PhysicsComp->GetComponentTransform();
	PauseMovement();
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathParticle, Spawn);
	InvalidatePlatform(GroundPlatform, CachedGroundPlatform);
	InvalidatePlatform(TransportPlatform, CachedTransportPlatform);
	InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform);
}

void ADimenseCharacter::Respawn(){
	bCanMoveAround = true;
	bCanTransport = true;
	PhysicsComp->SetWorldLocation(GroundLocation+FVector(0.0f,0.0f,MyHeight/2));
	GetCharacterMovement()->GravityScale = 1.0f;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

FVector ADimenseCharacter::RoundVector(const FVector& Vector) const{
	return FVector(FMath::RoundToInt(Vector.X), FMath::RoundToInt(Vector.Y), FMath::RoundToInt(Vector.Z));
}

void ADimenseCharacter::ResetCanMoveAround_Implementation() {}
void ADimenseCharacter::StartCanMoveAroundTimer_Implementation() {}
void ADimenseCharacter::ResetCanTransport_Implementation() {}
void ADimenseCharacter::StartCanTransportTimer_Implementation() {}

/*
bool ADimenseCharacter::SingleBoxTrace(UPARAM(ref) FHitResult& HitResult, const FVector& Start, const FVector& End, const FCollisionShape& Box) const {
	GetWorld()->SweepSingleByChannel(HitResult, Start, End, MainCamera->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams);
	if (HitResult.IsValidBlockingHit()) {
		return true;
	}
	return false;
}
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init and Debug ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Called to bind functionality to input
void ADimenseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent){
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ADimenseCharacter::InitDebug(){
	//Enable/Disable debug printing, views, control panel, etc.
	if (!bDebug) {
		bDebugPlatformRemote = false;
	}
}

void ADimenseCharacter::Debug() const{
	FString TransportPlatformText = TEXT("TransportPlatform: ");
	FString GroundPlatformText = TEXT("GroundPlatform: ");
	FString MoveAroundPlatformText = TEXT("MoveAroundPlatform: ");
	FString CachedMoveAroundPlatformText = TEXT("MoveAroundPlatform_CACHED: ");
	FString CachedTransportPlatformText = TEXT("TransportPlatform_CACHED: ");
	FString CachedGroundPlatformText = TEXT("GroundPlatform_CACHED: ");
	bool bIsFalling = GetCharacterMovement()->IsFalling();

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, (TEXT(""))); //Blank Line for Spacing
	//Cached Move Around Edge Platform debug
	if (CachedMoveAroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("%s"), CachedMoveAroundPlatformText + CachedMoveAroundPlatform->GetName()));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), CachedMoveAroundPlatformText + FString(TEXT("NULL"))));
	}
	//Move Around Edge Platform debug
	if (MoveAroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), MoveAroundPlatformText + MoveAroundPlatform->GetName()));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), MoveAroundPlatformText + FString(TEXT("NULL"))));
	}
	//Transport Platform debug
	if (TransportPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), TransportPlatformText + TransportPlatform->GetName()));
	}else if (CachedTransportPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("%s"), CachedTransportPlatformText + CachedTransportPlatform->GetName()));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), TransportPlatformText + FString(TEXT("NULL"))));
	}
	//Recent Ground Platform debug
	if (GroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), GroundPlatformText + GroundPlatform->GetName()));
	}else if (CachedGroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("%s"), CachedGroundPlatformText + CachedGroundPlatform->GetName()));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), GroundPlatformText + FString(TEXT("NULL"))));
	}
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, (TEXT(""))); //Blank Line for Spacing
	if (bSpinning) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("Spinning: true")));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Spinning: false")));
	}
	//Can Move Around Edge debug
	if (bCanMoveAround) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("CanMoveAround: true")));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("CanMoveAround: false")));
	}
	//Can Transport debug
	if (bCanTransport) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("CanTransport: true")));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("CanTransport: false")));
	}
	//Is Falling debug
	if (bIsFalling) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("OnGround: false")));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("OnGround: true")));
	}
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, (TEXT(""))); //Blank Line for Spacing
	//Cam Sign debug
	if (CamSign == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString(TEXT("CameraSign: ")) + FString::FromInt(CamSign)));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString(TEXT("CameraSign: ")) + FString::FromInt(CamSign)));
	}
	//Cam Side debug
	if (CamSide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString(TEXT("CameraSide: ")) + FString::FromInt(CamSide)));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString(TEXT("CameraSide: ")) + FString::FromInt(CamSide)));
	}
	//Visibility Side debug
	if (VisibilitySide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString(TEXT("Visibility: ")) + FString::FromInt(VisibilitySide)));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString(TEXT("Visibility: ")) + FString::FromInt(VisibilitySide)));
	}
	//Movement Direction debug
	if (MovementDirection == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Moving Right")));
	}else if (MovementDirection == -1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Moving Left")));
	}else{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("Not Moving")));
	}
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, (TEXT("%s"), FString(TEXT("FootLocation: ")) + FootLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, (TEXT("%s"), FString(TEXT("HeadLocation: ")) + HeadLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, (TEXT("%s"), FString(TEXT("GroundLocation: ")) + *GroundLocation.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, (TEXT("%s"), FString(TEXT("CameraForwardVector: ")) + *CamForwardVector.ToString()));
}