// Copyright Ryan Gourley 2019

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
#include "DrawDebugHelpers.h"
#include "DimensePlayerController.h"
#include "PlatformMaster.h"

// Sets default values
ADimenseCharacter::ADimenseCharacter()
{
	//Unreal Variables
	PrimaryActorTick.bCanEverTick = true;

	//My Variables
	bDebug = true; //Is debug print, views, control panel, etc. enabled?
	bDebugPlatformRemote = true; //Is debug drawing for platform checks by platforms?
	bCanMoveAroundEdge = true; //Can the movement system move the character around walls?
	bCanDimense = true; //Can the movement system move the character to platforms "based on a 2 dimensional view"?
	bSpinning = false; //Is the camera actively spinning to a new 90 degree view?
	PhysicsComp = GetCapsuleComponent(); //Set the physics component
	MyHeight = PhysicsComp->GetScaledCapsuleHalfHeight() * 2; //Player Height
	MyWidth = PhysicsComp->GetScaledCapsuleRadius(); //Player Width
	DefaultSpringArmLength = 200000; //Distance from camera to the player
	SideTraceLength = 50; //Length of lines drawn to detect walls/edges to move around
	LandingOffsetPadding = FVector(50, 50, 0); //Distance used to correct player position when moved by the movement system
	WalkAcceleration = 16; //Acceleration speed while walking
	GroundTraceLength = 2; //Length of the line used to detect the ground
	DeathDistance = 2500; //Distance you can fall before being reset
	MeshRotationTime = .05; //Time it takes for the mesh to correct rotation for a new 90 degree view
	CameraZoomTime = 2; //Time it takes for the camera to zoom into position when a map is loaded
	GroundAndHeadTraceRadiusMultiplier = 2.5; //Magic Number?
	DimenseTraceZOffset = 50; //Offset Z for Dimense line trace

	//Debug Trace Tagging
	bDebugDimenseTrace = 0;
	bDebugMoveAroundEdge = 0;
	bDebugSidesTrace = 0;
	bDebugVisibilityTrace = 0;
	SidesTraceTag = FName("SidesTraceTag");
	DimenseTraceTag = FName("DimenseTraceTag");
	VisibilityTraceTag = FName("VisibilityTraceTag");
	MoveAroundEdgeTraceTag = FName("MoveAroundEdgeTraceTag");
	SidesQParams = FCollisionQueryParams(SidesTraceTag, false, this);
	//SidesQParams.bReturnPhysicalMaterial = false;
	DimenseQParams = FCollisionQueryParams(DimenseTraceTag, false, this);
	//DimenseQParams.bReturnPhysicalMaterial = false;
	VisibilityQParams = FCollisionQueryParams(VisibilityTraceTag, false, this);
	//VisibilityQParams.bReturnPhysicalMaterial = false;
	MoveAroundQParams = FCollisionQueryParams(MoveAroundEdgeTraceTag, false, this);
	//MoveAroundQParams.bReturnPhysicalMaterial = false;

	//SubObjects------------------------------------------------------------------------------------------------------------------>>>
	//Main Spring Arm for the whole camera chassis.
	RotationSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("RotationSpringArm"));
	RotationSpringArm->SetupAttachment(RootComponent, FName("RotationSpringArm"));
	RotationSpringArm->bDoCollisionTest = false;
	RotationSpringArm->bEnableCameraLag = false;
	//Spring Arm for the main camera
	MainCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MainCameraSpringArm"));
	MainCameraSpringArm->SetupAttachment(RotationSpringArm, FName("MainCameraSpringArm"));
	MainCameraSpringArm->bDoCollisionTest = false;
	MainCameraSpringArm->bEnableCameraLag = true;
	MainCameraSpringArm->TargetArmLength = DefaultSpringArmLength;
	//Main Camera
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(MainCameraSpringArm, FName("MainCamera"));
	MainCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	MainCamera->FieldOfView = .1;
	//AntiCamera is used as a point of reference for the location opposite of the camera (where the MainCamera would be if the Spring Arm was rotated 180 degrees)
	AntiCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("AntiCameraSpringArm"));
	AntiCameraSpringArm->SetupAttachment(RotationSpringArm, FName("AntiCameraSpringArm"));
	AntiCameraSpringArm->bDoCollisionTest = false;
	AntiCameraSpringArm->bEnableCameraLag = true;
	AntiCameraSpringArm->TargetArmLength = MainCameraSpringArm->TargetArmLength * -1;
	//Scene component for point of reference
	AntiCameraSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AntiCameraSceneComponent"));
	AntiCameraSceneComponent->SetupAttachment(AntiCameraSpringArm, FName("AntiCameraSceneComponent"));
	//Spring Arm for the isometric debug window
	PnPCaptureSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PnPCaptureSpringArm"));
	PnPCaptureSpringArm->SetupAttachment(RotationSpringArm, FName("PnPCaptureSpringArm"));
	PnPCaptureSpringArm->SetRelativeRotation(FRotator(-30.0, -45.0, 0));
	PnPCaptureSpringArm->bDoCollisionTest = false;
	PnPCaptureSpringArm->bEnableCameraLag = false;
	PnPCaptureSpringArm->TargetArmLength = 180000;
	//<<<------------------------------------------------------------------------------------------------------------------SubObjects

	//Set the length of the line used for platform testing to the distance of the camera from the player
	FromCameraLineLength = MainCameraSpringArm->TargetArmLength;
	//A vector based on FromCameraLineLength, typically multiplied by a CamForwardVector/CamRightVector (so either x or y becomes 0, based on 90 degree angles)
	FromCameraLineVector = FVector(FromCameraLineLength, FromCameraLineLength, 0);

	if (!bDebug) {
		bDebugPlatformRemote = false;
	}
}

// Called when the game starts or when spawned
void ADimenseCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Enable/Disable debug printing, views, control panel, etc.
	if (bDebug) {
		if (bDebugSidesTrace) {
			GetWorld()->DebugDrawTraceTag = SidesTraceTag;
		}
		if (bDebugVisibilityTrace) {
			GetWorld()->DebugDrawTraceTag = VisibilityTraceTag;
		}
		if (bDebugDimenseTrace) {
			GetWorld()->DebugDrawTraceTag = DimenseTraceTag;
		}
		/*if (bDebugMoveAroundEdge) {
			GetWorld()->DebugDrawTraceTag = MoveAroundEdgeTraceTag;
		}*/
	}

	DefaultJumpZVelocity = GetCharacterMovement()->JumpZVelocity; //Store the JumpZVelocity
	GetMesh()->SetRenderCustomDepth(true); //Enable depth rendering for when you are behind objects (Stencil and Material need to be setup as well)
}

// Called to bind functionality to input
void ADimenseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// Called every frame
void ADimenseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//Keep track of the order of things here. The following order makes the most sense.
	//1: update variables (CamForwardVector, CamRightVector, CamSign/CamSide, Head/FootLocation --- These are used for the following calculations)
	//2: add movement (needs to happen before platform checks so they can happen in the same frame)
	//3: line traces and platform checks (check visibility, and for MoveAroundEdge and Dimense cases)
	//4: set ground location if on platform (need the platform first)
	//5: rotate the character mesh to the player movement (this could change if platform checks moves the character, so it should be after)

	if (!bSpinning) {
		//Some variables need to be updated every frame, but only while not spinning/rotating camera because movement is paused
		UpdateTickMovementVariables(); //This should remain at the beginning of tick
	}

	//Player movement
	AddMovementInput(GetMovementVectorFRI());
	//Line traces used by the movement system to determine when and where to move the player from/to platforms
	if (!bSpinning) {
		DoLineTracesAndPlatformChecks();
	}

	if (RecentGroundPlatform) { //If valid ground platform found
		RecentGroundLocation = PhysicsComp->GetComponentLocation(); //Set the ground location
	}

	if (RecentGroundPlatform || FacingDirection != 0) {
		RotateMeshToMovement();
	}

	//Enable/Disable custom debug drawing (Platform outlines for example)
	if (bDebug) {
		Debug();
	}
}

void ADimenseCharacter::UpdateTickMovementVariables()
{
	//The Cam* variables are used by the movement system to determine which vectors apply based on the camera angle, and also direction based on +/- values
	CamForwardVector = RoundVector(FVector(MainCamera->GetForwardVector()));
	CamRightVector = RoundVector(FVector(MainCamera->GetRightVector()));
	CamSign = FMath::Sign(CamRightVector.Y + CamRightVector.X);
	if (!FMath::IsNearlyZero(double(CamForwardVector.X), 0.1)) {
		CamSide = 1;
	}
	else {
		CamSide = -1;
	}

	//Head and foot location used by the movement system
	FootLocation = PhysicsComp->GetComponentLocation() - FVector(0, 0, MyHeight) / 2;;
	HeadLocation = FootLocation + FVector(0, 0, MyHeight);
}

void ADimenseCharacter::DoLineTracesAndPlatformChecks()
{
	FindVisibilitySide();

	//Check if the player is on the ground...
	if (GroundHitCheck()) { //if you are on the ground
		SetRecentGroundPlatform();
	}
	else { //if you are not on the ground
		InvalidateRecentGroundPlatform();

		//If you are falling downward, try to Dimense
		if (IsFallingDownward()) {
			//InvalidateDimensePlatform(); //This could be invalidating before it should
			TryDimense();
		}
		else {
			//Check if there is something above and if so, move "around" it (forward or backward)
			if (HeadHitCheck()) {
				TryMoveAroundEdge(HeadHitResult, FVector(0.f, 0.f, 5.f));
			}
		}
	}
	//Check if the player is moving to the right
	if (PhysicsComp->GetComponentVelocity().X * CamSign > 0 || PhysicsComp->GetComponentVelocity().Y * CamSign > 0) {
		MovementDirection = 1;
		//Check if there is something to the right and if so, move "around" it (forward or backward)
		if (RightHitCheck()) {
			TryMoveAroundEdge(RightHitResult);
		}
	}
	//Check if the player is moving to the left
	else if (PhysicsComp->GetComponentVelocity().X * CamSign < 0 || PhysicsComp->GetComponentVelocity().Y * CamSign < 0) {
		MovementDirection = -1;
		//Check if there is something to the left and if so, move "around" it (forward or backward)
		if (LeftHitCheck()) {
			TryMoveAroundEdge(LeftHitResult);
		}
	}
	//Check if the player is not moving
	else if (PhysicsComp->GetComponentVelocity().X == 0 && PhysicsComp->GetComponentVelocity().Y == 0) {
		MovementDirection = 0;
	}
}

FVector ADimenseCharacter::GetDimenseOffset(APlatformMaster* Platform, bool bDebugLocal)
{
	FVector Origin; FVector Extent; Platform->GetActorBounds(true, Origin, Extent);
	FVector Offset = FVector(DimenseHitResult.Location.X, DimenseHitResult.Location.Y, Origin.Z + Extent.Z) - PhysicsComp->GetComponentLocation();
	FVector DimenseOffset = CamForwardVector.GetAbs() * (Offset + (LandingOffsetPadding.X * CamSide * CamSign * VisibilitySide));
	if (bDebug && bDebugLocal) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Green, false, .5, 100, 10);
	}
	return DimenseOffset;
}

FVector ADimenseCharacter::GetMoveAroundEdgeOffset(FVector Location)
{
	FVector Offset = Location - PhysicsComp->GetComponentLocation();
	FVector WorldOffset = FVector(1, 1, 0) * CamForwardVector * VisibilitySide * CamSide * CamSign * (Offset - LandingOffsetPadding * CamSide * CamSign);

	if (bDebug) {
		FVector Origin; FVector Extent; MoveAroundEdgePlatform->GetActorBounds(true, Origin, Extent);
		DrawDebugDirectionalArrow(GetWorld(), PhysicsComp->GetComponentLocation() - FVector(0, 0, MyHeight / 2), PhysicsComp->GetComponentLocation() - FVector(0, 0, MyHeight / 2) + WorldOffset, 500, FColor::Orange, false, 5, 54, 3);
		//DrawDebugSphere(GetWorld(), HitResult.Location, 10, 4, FColor::Yellow, false, 3, 0, 10);
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Orange, false, .5, 97, 10);
	}
	return WorldOffset;
}


void ADimenseCharacter::ResetCanMoveAroundEdge_Implementation()
{
}

void ADimenseCharacter::StartCanMoveAroundEdgeTimer_Implementation()
{
}

void ADimenseCharacter::ResetCanDimense_Implementation()
{
}

void ADimenseCharacter::StartCanDimenseTimer_Implementation()
{
}

void ADimenseCharacter::InvalidateRecentGroundPlatform()
{
	if (RecentGroundPlatform) {
		if (bDebug) {
			FVector Origin;	FVector Extent;	RecentGroundPlatform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, FColor::Red, false, .25, 2, 10);
		}
		CachedGroundPlatform = RecentGroundPlatform;
		RecentGroundPlatform = nullptr;
	}
}

void ADimenseCharacter::InvalidateDimensePlatform()
{
	if (DimensePlatform) {
		/*
		if (bDebug) {
			FVector Origin; FVector Extent;	DimensePlatform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, FColor::Magenta, false, .25, 1, 10);
		}
		*/
		CachedDimensePlatform = DimensePlatform;
		DimensePlatform = nullptr;
	}
}

void ADimenseCharacter::InvalidateMoveAroundEdgePlatform()
{
	if (MoveAroundEdgePlatform) {
		/*
		if (bDebug) {
			FVector Origin; FVector Extent;	MoveAroundEdgePlatform->GetActorBounds(true, Origin, Extent);
			//DrawDebugBox(GetWorld(), Origin, Extent, FColor::Silver, false, .25, 0, 10);
		}
		*/
		CachedMoveAroundEdgePlatform = MoveAroundEdgePlatform;
		MoveAroundEdgePlatform = nullptr;
	}
}

void ADimenseCharacter::InvalidateCachedMoveAroundEdgePlatform()
{
	if (CachedMoveAroundEdgePlatform) {
		/*
		if (bDebug) {
			FVector Origin;
			FVector Extent;
			MoveAroundEdgePlatform->GetActorBounds(true, Origin, Extent);
			//DrawDebugBox(GetWorld(), Origin, Extent, FColor::Silver, false, .25, 0, 10);
		}
		*/
		CachedMoveAroundEdgePlatform = nullptr;
	}
}


void ADimenseCharacter::Dimense()
{
	PhysicsComp->AddWorldOffset(GetDimenseOffset(DimensePlatform, true));
	InvalidateMoveAroundEdgePlatform();
	//StartCanMoveAroundEdgeTimer();
}

void ADimenseCharacter::MoveAroundEdge()
{
	GEngine->AddOnScreenDebugMessage(-1, .5, FColor::Red, (TEXT("MoveAroundEdge")));

	PhysicsComp->AddWorldOffset(GetMoveAroundEdgeOffset(MoveAroundEdgeHitResult.Location));
	InvalidateDimensePlatform();
	//InvalidateMoveAroundEdgePlatform();
	//StartCanDimenseTimer();
}

//change to very short/thin, but wide as the player *box trace*?
bool ADimenseCharacter::LineTraceForDimenseHit(float ZOffset)
{
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide * -1;
	FVector FootZ = FootLocation - FVector(0, 0, ZOffset);
	FVector Start = FootZ + LineVector;
	FVector End = FootZ - LineVector;
	if (bDebugDimenseTrace) {
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, .01, 0, 2);
	}
	return SingleTrace(DimenseHitResult, Start, End, DimenseQParams);
}

//Used if the side checks (inluding ground and head) detect an object - Draw a new line from the player to the location of the sides' HitResult 
//(in theory this line passes through the object at the location we want to move the player towards)
bool ADimenseCharacter::LineTraceForMoveAroundEdgeHit(FHitResult& HitResult, FVector Offset)
{
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide;
	FVector Start = HitResult.Location - LineVector + Offset;
	FVector End = HitResult.Location + Offset;
	if (bDebugMoveAroundEdge) {
		DrawDebugLine(GetWorld(), Start, End, FColor::Purple, false, .5, 0, 2);
	}
	return SingleTrace(MoveAroundEdgeHitResult, Start, End, MoveAroundQParams);
}

bool ADimenseCharacter::FindDimensePlatform()
{
	if (LineTraceForDimenseHit(DimenseTraceZOffset)) {
		if (SetDimensePlatform()) {
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::FindMoveAroundEdgePlatform(FHitResult& HitResult, FVector Offset)
{
	if (LineTraceForMoveAroundEdgeHit(HitResult, Offset)) {
		if (SetMoveAroundEdgePlatform()) {
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::SetRecentGroundPlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(GroundHitResult.GetActor());
	if (NewPlatform) {
		if (RecentGroundPlatform != NewPlatform) {
			InvalidateRecentGroundPlatform();
			RecentGroundPlatform = NewPlatform;
			if (bDebug) {
				FVector Origin; FVector Extent; RecentGroundPlatform->GetActorBounds(true, Origin, Extent);
				DrawDebugBox(GetWorld(), Origin, Extent, FColor::FromHex("5D3900FF"), false, .5, 96, 5);  //Brown
			}
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::SetDimensePlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(DimenseHitResult.GetActor());
	if (NewPlatform) {
		if (DimensePlatform != NewPlatform) {
			if (PlayerAbovePlatformCheck(NewPlatform)) {
				if (!NewPlatform->PlatformAbovePlatformCheck(FootLocation)) {
					InvalidateDimensePlatform();
					DimensePlatform = NewPlatform;
					return true;
				}
			}
		}
	}
	return false;
}

bool ADimenseCharacter::SetMoveAroundEdgePlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(MoveAroundEdgeHitResult.GetActor());
	if (NewPlatform) {
		InvalidateMoveAroundEdgePlatform();
		MoveAroundEdgePlatform = NewPlatform;
		return true;
	}
	return false;
}

void ADimenseCharacter::TryDimense()
{
	if (bCanDimense) {
		if (FindDimensePlatform()) {
			Dimense();
		}
	}
}

void ADimenseCharacter::TryMoveAroundEdge(FHitResult& HitResult, FVector Offset)
{
	if (bCanMoveAroundEdge) {
		if (FindMoveAroundEdgePlatform(HitResult, Offset)) {
			MoveAroundEdge();
		}
	}
}

bool ADimenseCharacter::PlayerAbovePlatformCheck(APlatformMaster* Platform)
{
	FVector Origin;
	FVector Extent;
	Platform->GetActorBounds(true, Origin, Extent);
	FVector Top = Origin + FVector(0, 0, Extent.Z);
	if (FootLocation.Z >= Top.Z) {
		return true;
	}
	if (bDebug) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::FromHex("0081FFFF"), false, .5, 2, 10); //Blue
	}
	return false;
}

bool ADimenseCharacter::VisibilityCheck(FVector Start)
{
	int HitCount = 0;
	float Distance = MainCameraSpringArm->TargetArmLength;
	float X = CamSign * PhysicsComp->Bounds.BoxExtent.X;
	float Y = CamSign * PhysicsComp->Bounds.BoxExtent.Y;
	FVector CharacterLocation = PhysicsComp->GetComponentLocation();
	FVector End;

	End = FootLocation;
	if (SingleTrace(FrontHitResult, Start, End, VisibilityQParams)) {
		HitCount++;
	}
	End = HeadLocation;
	if (SingleTrace(FrontHitResult, Start, End, VisibilityQParams)) {
		HitCount++;
	}
	End = CharacterLocation + FVector(X, Y, 0);
	if (SingleTrace(FrontHitResult, Start, End, VisibilityQParams)) {
		HitCount++;
	}
	if (HitCount > 2) {
		return false;
	}
	End = CharacterLocation - FVector(X, Y, 0);
	if (SingleTrace(FrontHitResult, Start, End, VisibilityQParams)) {
		HitCount++;
	}
	if (HitCount > 2) {
		return false;
	}
	return true;
}

bool ADimenseCharacter::GroundHitCheck()
{
	float X = CamSign * PhysicsComp->Bounds.BoxExtent.X * GroundAndHeadTraceRadiusMultiplier;
	float Y = CamSign * PhysicsComp->Bounds.BoxExtent.Y * GroundAndHeadTraceRadiusMultiplier;
	float Distance = GroundTraceLength;
	float ZOffset = 1;

	FVector Start = FootLocation - FVector(X, Y, ZOffset);
	FVector End = Start - FVector(0, 0, Distance);
	if (SingleTrace(GroundHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("GroundHit0"));
		return true;
	}

	Start = FootLocation - FVector(X, -Y, ZOffset);
	End = Start - FVector(0, 0, Distance);
	if (SingleTrace(GroundHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("GroundHit1"));
		return true;
	}

	Start = FootLocation + FVector(X, -Y, -ZOffset);
	End = Start - FVector(0, 0, Distance);
	if (SingleTrace(GroundHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("GroundHit2"));
		return true;
	}

	Start = FootLocation + FVector(X, Y, -ZOffset);
	End = Start - FVector(0, 0, Distance);
	if (SingleTrace(GroundHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("GroundHit3"));
		return true;
	}
	return false;
}

bool ADimenseCharacter::HeadHitCheck()
{
	float X = CamSign * PhysicsComp->Bounds.BoxExtent.X * GroundAndHeadTraceRadiusMultiplier;
	float Y = CamSign * PhysicsComp->Bounds.BoxExtent.Y * GroundAndHeadTraceRadiusMultiplier;
	float Distance = SideTraceLength;
	float ZOffset = 1;

	FVector Start = HeadLocation - FVector(X, Y, -ZOffset);
	FVector End = Start + FVector(0, 0, Distance);
	if (SingleTrace(HeadHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("TopHit0"));
		return true;
	}

	Start = HeadLocation - FVector(X, -Y, -ZOffset);
	End = Start + FVector(0, 0, Distance);
	if (SingleTrace(HeadHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("TopHit1"));
		return true;
	}

	Start = HeadLocation + FVector(X, -Y, ZOffset);
	End = Start + FVector(0, 0, Distance);
	if (SingleTrace(HeadHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("TopHit2"));
		return true;
	}

	Start = HeadLocation + FVector(X, Y, ZOffset);
	End = Start + FVector(0, 0, Distance);
	if (SingleTrace(HeadHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("TopHit3"));
		return true;
	}
	return false;
}

bool ADimenseCharacter::LeftHitCheck()
{
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();
	float X = PhysicsComp->Bounds.BoxExtent.X * CamSign;
	float Y = PhysicsComp->Bounds.BoxExtent.Y * CamSign;
	float XMult = CamForwardVector.X * PhysicsComp->Bounds.BoxExtent.X * 2;
	float YMult = CamForwardVector.Y * PhysicsComp->Bounds.BoxExtent.Y * 2;
	float ZOffset = 0;

	Start = FootLocation;// -FVector(X, Y, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit0"));
		return true;
	}

	Start = FootLocation;// -FVector(X - XMult, Y + YMult, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit1"));
		return true;
	}

	Start = Center;// -FVector(X, Y, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit2"));
		return true;
	}

	Start = Center;// -FVector(X - XMult, Y + YMult, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit3"));
		return true;
	}

	Start = HeadLocation;// -FVector(X, Y, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit4"));
		return true;
	}

	Start = HeadLocation;// -FVector(X - XMult, Y + YMult, ZOffset);
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit5"));
		return true;
	}
	return false;
}

bool ADimenseCharacter::RightHitCheck()
{
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();
	float X = PhysicsComp->Bounds.BoxExtent.X * CamSign;
	float Y = PhysicsComp->Bounds.BoxExtent.Y * CamSign;
	float XMult = CamForwardVector.X * PhysicsComp->Bounds.BoxExtent.X * 2;
	float YMult = CamForwardVector.Y * PhysicsComp->Bounds.BoxExtent.Y * 2;
	float ZOffset = 0;

	Start = FootLocation;// +FVector(X, Y, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit0"));
		return true;
	}

	Start = FootLocation;// +FVector(X - XMult, Y + YMult, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit1"));
		return true;
	}

	Start = Center;// +FVector(X, Y, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit2"));
		return true;
	}

	Start = Center;// +FVector(X - XMult, Y + YMult, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit3"));
		return true;
	}

	Start = HeadLocation;// +FVector(X, Y, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit4"));
		return true;
	}

	Start = HeadLocation;// +FVector(X - XMult, Y + YMult, ZOffset);
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, SidesQParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit5"));
		return true;
	}
	return false;
}

int ADimenseCharacter::MoveLeftRight(float AxisValue)
{
	GetCharacterMovement()->AddInputVector(CamRightVector * AxisValue);
	FacingDirection = FMath::Sign(AxisValue);
	return FacingDirection;
}

void ADimenseCharacter::JumpDown()
{
	InvalidateDimensePlatform();
	if (RecentGroundPlatform) {
		if (FindDimensePlatform()) {
			FVector Offset = FVector(1, 1, 0) * (DimenseHitResult.Location - PhysicsComp->GetComponentLocation() - LandingOffsetPadding * CamForwardVector * VisibilitySide);
			PhysicsComp->AddWorldOffset(Offset);
			StartCanDimenseTimer();
		}
	}
}

bool ADimenseCharacter::IsFallingDownward()
{
	return PhysicsComp->GetComponentVelocity().Z <= 0;
}

void ADimenseCharacter::FindVisibilitySide()
{
	//If something is in between the camera and player: 1 if player is visible, -1 if visible from the back, 0 if not visible from either side
	if (VisibilityCheck(MainCamera->GetComponentLocation())) { //visible from the front?
		VisibilitySide = 1;
		//GetMesh()->SetRenderCustomDepth(true);  Always ON
	}
	else {
		VisibilitySide = -1;
		//GetMesh()->SetRenderCustomDepth(false);  Always ON
		if (!VisibilityCheck(AntiCameraSceneComponent->GetComponentLocation())) { //visible from the back side?
			VisibilitySide = 0;
		}
	}
}

void ADimenseCharacter::RotateCamera(float Rotation)
{
	if (!bSpinning) {
		for (int i = 1; i < 5; i++) {
			if (UKismetMathLibrary::EqualEqual_RotatorRotator(RotationSpringArm->RelativeRotation, FRotator(0, i * 90, 0), .001)) {
				RotationSpringArmLatentInfo.ExecutionFunction = FName("ResumeMovement");
				RotationSpringArmLatentInfo.UUID = 123;
				RotationSpringArmLatentInfo.Linkage = 1;
				RotationSpringArmLatentInfo.CallbackTarget = this;
				UKismetSystemLibrary::MoveComponentTo(RotationSpringArm, RotationSpringArm->RelativeLocation, FRotator(0, i * 90 + Rotation, 0), true, true, .5, true, EMoveComponentAction::Move, RotationSpringArmLatentInfo);
				bSpinning = true;
				bCanMoveAroundEdge = false;
				bCanDimense = false;
				PauseMovement();
				InvalidateMoveAroundEdgePlatform();
				InvalidateCachedMoveAroundEdgePlatform();
				InvalidateDimensePlatform();
				return;
			}
		}
	}
	return;
}

void ADimenseCharacter::RotateMeshToMovement()
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(MainCamera->GetComponentLocation(), GetMesh()->GetComponentLocation());
	auto MeshLatentInfo = FLatentActionInfo();
	MeshLatentInfo.CallbackTarget = GetMesh();
	UKismetSystemLibrary::MoveComponentTo(GetMesh(), GetMesh()->RelativeLocation, FRotator(0, (LookAtRotation.Yaw) + 90 + -90 * FacingDirection, 0), true, true, MeshRotationTime, false, EMoveComponentAction::Move, MeshLatentInfo);
}

void ADimenseCharacter::PauseMovement()
{
	DisableInput(GetWorld()->GetFirstPlayerController());

	CachedJumpKeyHoldTime = JumpKeyHoldTime;

	CachedComponentVelocity = PhysicsComp->GetPhysicsLinearVelocity();
	CachedCharacterMovementVelocity = GetCharacterMovement()->Velocity;

	PhysicsComp->SetAllPhysicsLinearVelocity(FVector(0, 0, 0));
	GetCharacterMovement()->Velocity = FVector(0, 0, 0);
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->JumpZVelocity = 0.0f;
}

void ADimenseCharacter::ResumeMovement()
{
	GetCharacterMovement()->GravityScale = 1;
	GetCharacterMovement()->JumpZVelocity = DefaultJumpZVelocity;
	GetCharacterMovement()->Velocity = CachedCharacterMovementVelocity;
	PhysicsComp->SetAllPhysicsLinearVelocity(CachedComponentVelocity);

	CachedComponentVelocity = FVector(0, 0, 0);
	CachedCharacterMovementVelocity = FVector(0, 0, 0);

	JumpKeyHoldTime = CachedJumpKeyHoldTime;
	CachedJumpKeyHoldTime = 0.0f;

	bSpinning = false;
	bCanMoveAroundEdge = true;
	bCanDimense = true;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

bool ADimenseCharacter::CheckDeath()
{
	if (DeathParticle) {
		FTransform Spawn = PhysicsComp->GetComponentTransform();
		//Spawn.SetScale3D(FVector(1, 1, 1));
		if (RecentGroundPlatform) {
			if (FootLocation.Z < RecentGroundPlatform->GetActorLocation().Z - DeathDistance) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathParticle, Spawn);
				PauseMovement();
				return true;
			}
		}
		else if (CachedGroundPlatform) {
			if (FootLocation.Z < CachedGroundPlatform->GetActorLocation().Z - DeathDistance) {
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathParticle, Spawn);
				PauseMovement();
				return true;
			}
		}
	}
	return false;
}

void ADimenseCharacter::Respawn()
{
	InvalidateDimensePlatform();
	if (RecentGroundPlatform) {
		FVector Origin;
		FVector Extent;
		FVector PlatformLocation = RecentGroundPlatform->GetActorLocation();
		RecentGroundPlatform->GetActorBounds(true, Origin, Extent);
		PhysicsComp->SetWorldLocation(RecentGroundLocation);
	}
	else if (CachedGroundPlatform) {
		FVector Origin;
		FVector Extent;
		FVector PlatformLocation = CachedGroundPlatform->GetActorLocation();
		CachedGroundPlatform->GetActorBounds(true, Origin, Extent);
		PhysicsComp->SetWorldLocation(RecentGroundLocation);
	}
	GetCharacterMovement()->GravityScale = 1;
	GetCharacterMovement()->JumpZVelocity = DefaultJumpZVelocity;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

FVector ADimenseCharacter::GetMovementVectorFRI() //(FRI = Frame Rate Independent)
{
	return CalculateTotalMovementInput(ConsumeMovementInputVector()) * GetWorld()->GetDeltaSeconds();
}

FVector ADimenseCharacter::CalculateTotalMovementInput(FVector Vector)
{
	return Vector * WalkAcceleration;
}

FVector ADimenseCharacter::RoundVector(FVector Vector)
{
	return FVector(FMath::RoundToInt(Vector.X), FMath::RoundToInt(Vector.Y), FMath::RoundToInt(Vector.Z));
}

bool ADimenseCharacter::SingleTrace(FHitResult &HitResult, FVector &Start, FVector &End, FCollisionQueryParams &QParams) {
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QParams);
	if (HitResult.IsValidBlockingHit()) {
		return true;
	}
	else {
		return false;
	}
}

void ADimenseCharacter::Debug()
{
	FString DimensePlatformText = "DimensePlatform: ";
	FString RecentGroundPlatformText = "RecentGroundPlatform: ";
	FString MoveAroundEdgePlatformText = "MoveAroundEdgePlatform: ";
	FString CachedMoveAroundEdgePlatformText = "CachedMoveAroundEdgePlatform: ";
	FString CachedDimensePlatformText = "DimensePlatform_CACHED: ";
	FString CachedRecentGroundPlatformText = "RecentGroundPlatform_CACHED: ";
	bool bIsFalling = GetCharacterMovement()->IsFalling();
	//DrawDebugBox(GetWorld(), FootLocation, FVector(5, 5, 5), FColor::Black, false, .5, 56, 2);

	//GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("RotationArm: ") + *(RotationSpringArm->RelativeRotation.ToString())));
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("CamForwardVector: ") + *CamForwardVector.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("RecentGroundLocation: ") + *RecentGroundLocation.ToString()));
	if (DimensePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), DimensePlatformText + DimensePlatform->GetName()));
	}
	else if (CachedDimensePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, (TEXT("%s"), CachedDimensePlatformText + CachedDimensePlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), DimensePlatformText + FString("NULL")));
	}
	if (RecentGroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), RecentGroundPlatformText + RecentGroundPlatform->GetName()));
	}
	else if (CachedGroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, (TEXT("%s"), CachedRecentGroundPlatformText + CachedGroundPlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), RecentGroundPlatformText + FString("NULL")));
	}
	if (MoveAroundEdgePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), MoveAroundEdgePlatformText + MoveAroundEdgePlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), MoveAroundEdgePlatformText + FString("NULL")));
	}
	if (CachedMoveAroundEdgePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), CachedMoveAroundEdgePlatformText + CachedMoveAroundEdgePlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), CachedMoveAroundEdgePlatformText + FString("NULL")));
	}
	if (bSpinning) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("Spinning: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("Spinning: false")));
	}
	if (bCanMoveAroundEdge) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("CanMoveAroundEdge: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("CanMoveAroundEdge: false")));
	}
	if (bCanDimense) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("CanDimense: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("CanDimense: false")));
	}
	if (bIsFalling) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("OnGround: false")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("OnGround: true")));
	}
	if (CamSign == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), FString("CamSign: ") + FString::FromInt(CamSign)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), FString("CamSign: ") + FString::FromInt(CamSign)));
	}
	if (CamSide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), FString("CamSide: ") + FString::FromInt(CamSide)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), FString("CamSide: ") + FString::FromInt(CamSide)));
	}
	if (VisibilitySide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("%s"), FString("Visibility: ") + FString::FromInt(VisibilitySide)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("%s"), FString("Visibility: ") + FString::FromInt(VisibilitySide)));
	}
	if (MovementDirection == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("Moving Right")));
	}
	else if (MovementDirection == -1) {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, (TEXT("Moving Left")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("Not Moving")));
	}
}