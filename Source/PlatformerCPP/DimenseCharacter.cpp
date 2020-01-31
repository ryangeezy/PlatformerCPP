// Copyright Ryan Gourley 2020

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
	bCanMoveAround = true; //Can the movement system move the character around walls?
	bCanDimense = true; //Can the movement system move the character to platforms "based on a 2 dimensional view"?
	bSpinning = false; //Is the camera actively spinning to a new 90 degree view?
	PhysicsComp = GetCapsuleComponent(); //Set the physics component
	MyHeight = 100.0f;//PhysicsComp->GetScaledCapsuleHalfHeight() * 2; //Player Height
	MyWidth = 40.0f;//PhysicsComp->GetScaledCapsuleRadius() * 2; //Player Width
	LandingOffsetPadding = FVector(MyWidth, MyWidth, 0.0f); //Distance used to correct player position when moved by the movement system
	DefaultSpringArmLength = 200000.0f; //Distance from camera to the player
	SideTraceLength = 25.0f; //Length of lines drawn to detect walls/edges to move around
	WalkAcceleration = 16.0f; //Acceleration speed while walking
	HeadTraceLength = 5.0f; //Length of the line used to detect the ground
	GroundTraceLength = 5.0f; //Length of the line used to detect the ground
	DeathDistance = 2500.0f; //Distance you can fall before being reset
	MeshRotationTime = .05f; //Time it takes for the mesh to correct rotation for a new 90 degree view
	CameraZoomTime = 2.0f; //Time it takes for the camera to zoom into position when a map is loaded
	DimenseTraceZOffset = 25.0f; //Offset Z for Dimense line trace

	//Debug Trace Tagging
	TraceTag = FName("TraceTag");
	QParams = FCollisionQueryParams(TraceTag, false, this);

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
	MainCamera->FieldOfView = 0.1f;
	//AntiCamera is used as a point of reference for the location opposite of the camera (where the MainCamera would be if the Spring Arm was rotated 180 degrees)
	AntiCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("AntiCameraSpringArm"));
	AntiCameraSpringArm->SetupAttachment(RotationSpringArm, FName("AntiCameraSpringArm"));
	AntiCameraSpringArm->bDoCollisionTest = false;
	AntiCameraSpringArm->bEnableCameraLag = true;
	AntiCameraSpringArm->TargetArmLength = MainCameraSpringArm->TargetArmLength * -1.0f;
	//Scene component for point of reference
	AntiCameraSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AntiCameraSceneComponent"));
	AntiCameraSceneComponent->SetupAttachment(AntiCameraSpringArm, FName("AntiCameraSceneComponent"));
	//Spring Arm for the isometric debug window
	PnPCaptureSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PnPCaptureSpringArm"));
	PnPCaptureSpringArm->SetupAttachment(RotationSpringArm, FName("PnPCaptureSpringArm"));
	PnPCaptureSpringArm->SetRelativeRotation(FRotator(-30.0f, -45.0f, 0.0f));
	PnPCaptureSpringArm->bDoCollisionTest = false;
	PnPCaptureSpringArm->bEnableCameraLag = false;
	PnPCaptureSpringArm->TargetArmLength = 180000.0f;
	//<<<------------------------------------------------------------------------------------------------------------------SubObjects

	//Set the length of the line used for platform testing to the distance of the camera from the player
	FromCameraLineLength = MainCameraSpringArm->TargetArmLength;
	//A vector based on FromCameraLineLength, typically multiplied by a CamForwardVector/CamRightVector (so either x or y becomes 0, based on 90 degree angles)
	FromCameraLineVector = FVector(FromCameraLineLength, FromCameraLineLength, 0.0f);
}

// Called when the game starts or when spawned
void ADimenseCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitDebug();
}

// Called every frame
void ADimenseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//Keep track of the order of things here. The following order makes the most sense.
	//1: update variables (CamForwardVector, CamRightVector, CamSign/CamSide, Head/FootLocation --- These are used for the following calculations)
	//2: add movement from input (needs to happen before platform checks so they can happen in the same frame, in that order)
	//3: line traces and platform checks (check visibility, and for MoveAround and Dimense cases)
	//4: set ground location if on platform (need the platform first)
	//5: rotate the character mesh to the player movement (this could change if platform checks moves the character, so it should be after)

	if (!bSpinning) {
		//Some variables need to be updated every frame, but only while not spinning/rotating camera because movement is paused
		UpdateTickMovementVariables(); //This should remain at the beginning of tick
		AddMovementInput(GetMovementInputVectorFRI()); //Player movement
		DoLineTracesAndPlatformChecks(); //Line traces used by the movement system to determine when and where to move the player from/to platforms
	}

	if (GroundPlatform) { //If valid ground platform found
		GroundLocation = PhysicsComp->GetComponentLocation(); //Set the ground location
	}

	if (GroundPlatform || FacingDirection != 0) {
		RotateMeshToMovement();
	}

	//Enable/Disable custom debug drawing (Platform outlines for example)
	if (bDebug) {
		Debug();
	}
}

void ADimenseCharacter::DoLineTracesAndPlatformChecks()
{
	FindVisibilitySide();
	SetMovementDirection();

	//Check if the player is on the ground...
	//if (VerticalBoxTrace(FootLocation, GroundHitResult, GroundTraceLength, -1, "Ground", true)) { //if you are on the ground
	//	SetGroundPlatform();
	//}
	//else { //if you are not on the ground
	//	InvalidatePlatform(GroundPlatform, CachedGroundPlatform, true);

		//If you are falling downward, try to Dimense
	//	if (IsFallingDownward()) {
	//		TryDimense();
	//	}
	//	else {
			//Check if there is something above and if so, move "around" it (forward or backward)
	//		if (VerticalBoxTrace(HeadLocation, HeadHitResult, HeadTraceLength, 1, "Head", true)) {
	//			TryMoveAround(HeadHitResult, FVector(0.0f, 0.0f, 5.0f));
	//		}
	//	}
	//}	

	//Check if the player is moving to the right
	if (MovementDirection == 1) {
		//Check if there is something to the right and if so, move "around" it (forward or backward)
		if (RightHitCheck()) {
			TryMoveAround(RightHitResult);
		}
	}
	//Check if the player is moving to the left
	else if (MovementDirection == -1) {
		//Check if there is something to the left and if so, move "around" it (forward or backward)
		if (LeftHitCheck()) {
			TryMoveAround(LeftHitResult);
		}
	}
}

bool ADimenseCharacter::TryDimense()
{
	if (bCanDimense) {
		if (LineTraceForDimenseHit(DimenseTraceZOffset)) {
			if (SetDimensePlatform()) {
				Dimense();
				return true;
			}
		}
	}
	return false;
}

//change to very short/thin, but wide as the player *box trace*?
bool ADimenseCharacter::LineTraceForDimenseHit(float ZOffset)
{
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide * -1;
	FVector FootZ = FootLocation - FVector(0.0f, 0.0f, ZOffset);
	FVector Start = FootZ + LineVector;
	FVector End = FootZ - LineVector;
	if (bDebug) {
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, .01f, 0, 2.0f);
	}
	return SingleTrace(DimenseHitResult, Start, End, QParams);
}

//Used if the side checks (inluding ground and head) detect an object - Draw a new line from the player to the location of the sides' HitResult 
//(in theory this line passes through the object at the location we want to move the player towards)
bool ADimenseCharacter::LineTraceForMoveAroundHit(FHitResult& HitResult, FVector Offset)
{
	FVector LineVector = FromCameraLineVector * CamForwardVector * VisibilitySide;
	FVector Start = HitResult.Location - LineVector + Offset;
	FVector End = HitResult.Location + Offset;
	if (bDebug) {
		DrawDebugLine(GetWorld(), Start, End, FColor::Purple, false, .5f, 0, 2.0f);
	}
	return SingleTrace(MoveAroundHitResult, Start, End, QParams);
}

bool ADimenseCharacter::SetDimensePlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(DimenseHitResult.GetActor());
	if (NewPlatform) {
		if (PlayerAbovePlatformCheck(NewPlatform)) {
			if (!NewPlatform->PlatformAbovePlatformCheck()) {
				InvalidatePlatform(DimensePlatform, CachedDimensePlatform, false);
				DimensePlatform = NewPlatform;
				return true;
			}
		}
	}
	return false;
}

bool ADimenseCharacter::SetGroundPlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(GroundHitResult.GetActor());
	if (NewPlatform) {
		if (GroundPlatform != NewPlatform) {
			InvalidatePlatform(GroundPlatform, CachedGroundPlatform, true);
			GroundPlatform = NewPlatform;
			if (bDebug) {
				FVector Origin; FVector Extent; GroundPlatform->GetActorBounds(true, Origin, Extent);
				DrawDebugBox(GetWorld(), Origin, Extent, FColor::FromHex("5D3900FF"), false, .5f, 96, 5.0f);  //Brown
			}
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::SetPlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, UPARAM(ref) AActor*& HitActor, FColor DebugColor, bool bDebugLocal)
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(HitActor);
	if (NewPlatform) {
		if (Platform != NewPlatform) {
			InvalidatePlatform(Platform, CachedPlatform, false);
			Platform = NewPlatform;
			if (bDebug && bDebugLocal) {
				FVector Origin; FVector Extent; GroundPlatform->GetActorBounds(true, Origin, Extent);
				DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, .5f, 96, 5.0f);
			}
			return true;
		}
	}
	return false;
}

bool ADimenseCharacter::SetMoveAroundPlatform()
{
	APlatformMaster* NewPlatform = Cast<APlatformMaster>(MoveAroundHitResult.GetActor());
	if (NewPlatform) {
		InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform,false);
		MoveAroundPlatform = NewPlatform;
		return true;
	}
	return false;
}

void ADimenseCharacter::Dimense()
{
	PhysicsComp->AddWorldOffset(GetDimenseOffset(DimensePlatform, true));
	InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform, false);
	//StartCanMoveAroundTimer();
}

FVector ADimenseCharacter::GetDimenseOffset(APlatformMaster* Platform, bool bDebugLocal) const
{
	FVector Origin; FVector Extent; Platform->GetActorBounds(true, Origin, Extent);
	FVector Offset = FVector(DimenseHitResult.Location.X, DimenseHitResult.Location.Y, Origin.Z + Extent.Z) - PhysicsComp->GetComponentLocation();
	FVector DimenseOffset = CamForwardVector.GetAbs() * (Offset + (LandingOffsetPadding.X * CamSide * CamSign * VisibilitySide));
	if (bDebug && bDebugLocal) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Green, false, .5f, 100, 10.0f);
	}
	return DimenseOffset;
}

bool ADimenseCharacter::PlayerAbovePlatformCheck(APlatformMaster* Platform)
{
	FVector Origin;
	FVector Extent;
	Platform->GetActorBounds(true, Origin, Extent);
	FVector Top = Origin + FVector(0.0f, 0.0f, Extent.Z);
	if (FootLocation.Z >= Top.Z) {
		return true;
	}
	if (bDebug) {
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::FromHex("0081FFFF"), false, .5f, 2, 10.0f); //Blue
	}
	return false;
}

bool ADimenseCharacter::TryMoveAround(FHitResult& HitResult, FVector Offset)
{
	if (bCanMoveAround) {
		if (LineTraceForMoveAroundHit(HitResult, Offset)) {
			if (SetMoveAroundPlatform()) {
				MoveAround();
				return true;
			}
		}
	}
	return false;
}

void ADimenseCharacter::MoveAround()
{
	GEngine->AddOnScreenDebugMessage(-1, .5f, FColor::Red, (TEXT("MoveAround")));

	PhysicsComp->AddWorldOffset(GetMoveAroundOffset(MoveAroundHitResult.Location));
	InvalidatePlatform(DimensePlatform, CachedDimensePlatform, false);
	//InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform, false);
	//StartCanDimenseTimer();
}

FVector ADimenseCharacter::GetMoveAroundOffset(FVector Location) const
{
	FVector Offset = Location - PhysicsComp->GetComponentLocation();
	FVector WorldOffset = FVector(1.0f, 1.0f, 0.0f) * CamForwardVector * VisibilitySide * CamSide * CamSign * (Offset - LandingOffsetPadding * CamSide * CamSign);

	if (bDebug) {
		FVector Origin; FVector Extent; MoveAroundPlatform->GetActorBounds(true, Origin, Extent);
		DrawDebugDirectionalArrow(GetWorld(), PhysicsComp->GetComponentLocation() - FVector(0.0f, 0.0f, MyHeight / 2), PhysicsComp->GetComponentLocation() - FVector(0.0f, 0.0f, MyHeight / 2) + WorldOffset, 500.0f, FColor::Orange, false, 5.0f, 54, 3.0f);
		//DrawDebugSphere(GetWorld(), HitResult.Location, 10, 4, FColor::Yellow, false, 3, 0, 10);
		DrawDebugBox(GetWorld(), Origin, Extent, FColor::Orange, false, .5f, 97, 10.0f);
	}
	return WorldOffset;
}

bool ADimenseCharacter::VerticalBoxTrace(FVector Location, FHitResult& HitResult, float TraceLength, int32 UpOrDown, FString DebugPhrase, bool bDebugLocal)
{
	//UpOrDown should be 1 or -1
	float X = CamSign * (MyWidth / 2);
	float Y = CamSign * (MyWidth / 2);
	FCollisionShape Box = FCollisionShape::MakeBox(FVector(X,Y,0.0f));

	FVector End = Location + (FVector(0.0f, 0.0f, TraceLength) * UpOrDown);
	if (GetWorld()->SweepSingleByChannel(HitResult, Location, End, PhysicsComp->GetComponentQuat(), ECollisionChannel::ECC_WorldStatic, Box, QParams)) {
		if (bDebug && bDebugLocal) {
			//GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Black, (TEXT("%s"), DebugPhrase + FString(" was hit")));
			FVector DebugBoxOffset = FVector(0.0f, 0.0f, (End.Z - Location.Z) / 2);
			DrawDebugBox(GetWorld(), Location + DebugBoxOffset, Box.GetExtent() + DebugBoxOffset, FColor::FromHex("261304FF"), false, .1f, 2, 1.0f); //Brown
		}
		return true;
	}
	if (bDebug && bDebugLocal) {
		FVector DebugBoxOffset = FVector(0.0f, 0.0f, (End.Z - Location.Z) / 2);
		DrawDebugBox(GetWorld(), Location + DebugBoxOffset, Box.GetExtent() + DebugBoxOffset, FColor::Red, false, .1f, 2, 1.0f);
	}
	return false;
}

bool ADimenseCharacter::HorizontalBoxTrace() {
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();

	//GetWorld()->SweepSingleByObjectType()
	return false;
}

bool ADimenseCharacter::HorizontalHitCheck(FVector Location, FHitResult &HitResult)
{
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();
	float X = CamSign * (MyWidth / 2);
	float Y = CamSign * (MyWidth / 2);

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		return true;
	}

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit4"));
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(HitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit5"));
		return true;
	}
	return false;
}

bool ADimenseCharacter::LeftHitCheck()
{
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();
	float X = CamSign * (MyWidth / 2);
	float Y = CamSign * (MyWidth / 2);

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit0"));
		return true;
	}

	Start = FootLocation;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit1"));
		return true;
	}
	
	Start = Center;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit2"));
		return true;
	}

	Start = Center;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit3"));
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit4"));
		return true;
	}

	Start = HeadLocation;
	End = Start + Distance;
	if (SingleTrace(LeftHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("LeftHit5"));
		return true;
	}
	return false;
}

bool ADimenseCharacter::RightHitCheck()
{
	FVector Start;
	FVector End;
	FVector Distance = FVector(SideTraceLength, SideTraceLength, 0.0f) * CamRightVector;
	FVector	Center = PhysicsComp->GetComponentLocation();
	float X = CamSign * (MyWidth / 2);
	float Y = CamSign * (MyWidth / 2);

	Start = FootLocation;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit0"));
		return true;
	}

	Start = FootLocation;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit1"));
		return true;
	}

	Start = Center;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit2"));
		return true;
	}

	Start = Center;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit3"));
		return true;
	}

	Start = HeadLocation;
	End = Start + Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit4"));
		return true;
	}

	Start = HeadLocation;
	End = Start - Distance;
	if (SingleTrace(RightHitResult, Start, End, QParams)) {
		//UE_LOG(LogTemp, Warning, TEXT("RightHit5"));
		return true;
	}
	return false;
}

int32 ADimenseCharacter::MoveLeftRight(float AxisValue)
{
	GetCharacterMovement()->AddInputVector(CamRightVector * AxisValue);
	FacingDirection = FMath::Sign(AxisValue);
	return FacingDirection;
}

void ADimenseCharacter::JumpDown()
{
	if (GroundPlatform) {
		InvalidatePlatform(DimensePlatform, CachedDimensePlatform, false);
		if (TryDimense()) {
			FVector Offset = FVector(1.0f, 1.0f, 0.0f) * (DimenseHitResult.Location - PhysicsComp->GetComponentLocation() - LandingOffsetPadding * CamForwardVector * VisibilitySide);
			PhysicsComp->AddWorldOffset(Offset);
			StartCanDimenseTimer();
		}
	}
}

bool ADimenseCharacter::IsFallingDownward()
{
	return PhysicsComp->GetComponentVelocity().Z <= 0.0f;
}


void ADimenseCharacter::InvalidatePlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, bool bDebugLocal, FColor DebugColor)
{
	
	if (Platform) {
		if (bDebug && bDebugLocal) {
			FVector Origin;	FVector Extent;	Platform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, .25f, 2, 10.0f);
		}
		CachedPlatform = Platform;
		Platform = nullptr;
	}	
}

void ADimenseCharacter::InvalidateCachedPlatform(UPARAM(ref) APlatformMaster*& CachedPlatform, bool bDebugLocal, FColor DebugColor)
{
	if (CachedPlatform) {
		if (bDebug && bDebugLocal) {
			FVector Origin;	FVector Extent;	CachedPlatform->GetActorBounds(true, Origin, Extent);
			DrawDebugBox(GetWorld(), Origin, Extent, DebugColor, false, .25f, 0, 10.0f);
		}
		CachedPlatform = nullptr;
	}
}

void ADimenseCharacter::FindVisibilitySide()
{
	//If something is in between the camera and player: 1 if player is visible, -1 if visible from the back, 0 if not visible from either side
	if (VisibilityCheck(MainCamera->GetComponentLocation())) { //visible from the front?
		if (!VisibilitySide) {
			TryDimense();
		}
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

bool ADimenseCharacter::VisibilityCheck(FVector Start)
{
	int32 HitCount = 0;
	float Distance = MainCameraSpringArm->TargetArmLength;
	float X = CamSign * PhysicsComp->Bounds.BoxExtent.X;
	float Y = CamSign * PhysicsComp->Bounds.BoxExtent.Y;
	FVector PlayerLocation = PhysicsComp->GetComponentLocation();
	FVector End;

	End = FootLocation;
	if (SingleTrace(FrontHitResult, Start, End, QParams)) {
		HitCount++;
	}
	End = HeadLocation;
	if (SingleTrace(FrontHitResult, Start, End, QParams)) {
		HitCount++;
	}
	End = PlayerLocation + FVector(X, Y, 0.0f);
	if (SingleTrace(FrontHitResult, Start, End, QParams)) {
		HitCount++;
	}
	if (HitCount > 2) {
		return false;
	}
	End = PlayerLocation - FVector(X, Y, 0.0f);
	if (SingleTrace(FrontHitResult, Start, End, QParams)) {
		HitCount++;
	}
	if (HitCount > 2) {
		return false;
	}
	return true;
}

void ADimenseCharacter::RotateCamera(float Rotation)
{
	if (!bSpinning) {
		for (int32 i = 1; i < 5; i++) {
			if (UKismetMathLibrary::EqualEqual_RotatorRotator(RotationSpringArm->RelativeRotation, FRotator(0.0f, i * 90.0f, 0.0f), .001f)) {
				RotationSpringArmLatentInfo.ExecutionFunction = FName("ResumeMovement");
				RotationSpringArmLatentInfo.UUID = 123;
				RotationSpringArmLatentInfo.Linkage = 1;
				RotationSpringArmLatentInfo.CallbackTarget = this;
				UKismetSystemLibrary::MoveComponentTo(RotationSpringArm, RotationSpringArm->RelativeLocation, FRotator(0.0f, i * 90.0f + Rotation, 0.0f), true, true, .5f, true, EMoveComponentAction::Move, RotationSpringArmLatentInfo);
				bSpinning = true;
				PauseMovement();
				InvalidatePlatform(GroundPlatform, CachedGroundPlatform, true);
				InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform, true);
				InvalidatePlatform(DimensePlatform, CachedDimensePlatform, true);
				return;
			}
		}
	}
}

void ADimenseCharacter::RotateMeshToMovement()
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(MainCamera->GetComponentLocation(), GetMesh()->GetComponentLocation());
	auto MeshLatentInfo = FLatentActionInfo();
	MeshLatentInfo.CallbackTarget = GetMesh();
	UKismetSystemLibrary::MoveComponentTo(GetMesh(), GetMesh()->RelativeLocation, FRotator(0.0f, (LookAtRotation.Yaw) + 90.0f + -90.0f * FacingDirection, 0.0f), true, true, MeshRotationTime, false, EMoveComponentAction::Move, MeshLatentInfo);
}

void ADimenseCharacter::PauseMovement()
{
	DisableInput(GetWorld()->GetFirstPlayerController());
	bCanDimense = false;
	bCanMoveAround = false;
	CachedJumpKeyHoldTime = JumpKeyHoldTime;
	CachedComponentVelocity = PhysicsComp->GetPhysicsLinearVelocity();
	CachedCharacterMovementVelocity = GetCharacterMovement()->Velocity;
	PhysicsComp->SetAllPhysicsLinearVelocity(FVector(0.0f, 0.0f, 0.0f));
	GetCharacterMovement()->Velocity = FVector(0.0f, 0.0f, 0.0f);
	GetCharacterMovement()->GravityScale = 0.0f;
}

void ADimenseCharacter::ResumeMovement()
{
	GetCharacterMovement()->GravityScale = 1.0f;
	GetCharacterMovement()->Velocity = CachedCharacterMovementVelocity;
	PhysicsComp->SetAllPhysicsLinearVelocity(CachedComponentVelocity);
	JumpKeyHoldTime = CachedJumpKeyHoldTime;
	bSpinning = false;
	bCanMoveAround = true;
	bCanDimense = true;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

void ADimenseCharacter::SetMovementDirection() {
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

FVector ADimenseCharacter::GetMovementInputVectorFRI() //(FRI = Frame Rate Independent)
{
	return CalculateTotalMovementInput(ConsumeMovementInputVector()) * GetWorld()->GetDeltaSeconds();
}

FVector ADimenseCharacter::CalculateTotalMovementInput(FVector Vector)
{
	return Vector * WalkAcceleration;
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
	FootLocation = PhysicsComp->GetComponentLocation() - (FVector(0.0f, 0.0f, MyHeight) / 2);
	HeadLocation = FootLocation + FVector(0.0f, 0.0f, MyHeight);
}

bool ADimenseCharacter::CheckDeath()
{
	if (DeathParticle) { //make sure the particle effect is selected in the blueprint
		if (GroundPlatform) {
			if (FootLocation.Z < GroundPlatform->GetActorLocation().Z - DeathDistance) {
				Die();
				return true;
			}
		}
		else if (CachedGroundPlatform) {
			if (FootLocation.Z < CachedGroundPlatform->GetActorLocation().Z - DeathDistance) {
				Die();
				return true;
			}
		}
	}
	return false;
}

void ADimenseCharacter::Die() 
{
	FTransform Spawn = PhysicsComp->GetComponentTransform();
	PauseMovement();
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathParticle, Spawn);
	InvalidatePlatform(GroundPlatform, CachedGroundPlatform, false);
	InvalidatePlatform(DimensePlatform, CachedDimensePlatform, false);
	InvalidatePlatform(MoveAroundPlatform, CachedMoveAroundPlatform, false);
}

void ADimenseCharacter::Respawn()
{
	bCanMoveAround = true;
	bCanDimense = true;
	PhysicsComp->SetWorldLocation(GroundLocation);
	GetCharacterMovement()->GravityScale = 1.0f;
	EnableInput(GetWorld()->GetFirstPlayerController());
}

bool ADimenseCharacter::SingleTrace(FHitResult& HitResult, FVector& Start, FVector& End, FCollisionQueryParams& QParams) {
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QParams);
	if (HitResult.IsValidBlockingHit()) {
		return true;
	}
	return false;
}

FVector ADimenseCharacter::RoundVector(FVector Vector) const
{
	return FVector(FMath::RoundToInt(Vector.X), FMath::RoundToInt(Vector.Y), FMath::RoundToInt(Vector.Z));
}

void ADimenseCharacter::ResetCanMoveAround_Implementation() const {}
void ADimenseCharacter::StartCanMoveAroundTimer_Implementation() const {}
void ADimenseCharacter::ResetCanDimense_Implementation() const {}
void ADimenseCharacter::StartCanDimenseTimer_Implementation() const {}

// Called to bind functionality to input
void ADimenseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ADimenseCharacter::InitDebug() 
{
	//Enable/Disable debug printing, views, control panel, etc.
	if (!bDebug) {
		bDebugPlatformRemote = false;
	}
}

void ADimenseCharacter::Debug() const
{
	FString DimensePlatformText = "DimensePlatform: ";
	FString GroundPlatformText = "GroundPlatform: ";
	FString MoveAroundPlatformText = "MoveAroundPlatform: ";
	FString CachedMoveAroundPlatformText = "CachedMoveAroundPlatform: ";
	FString CachedDimensePlatformText = "DimensePlatform_CACHED: ";
	FString CachedGroundPlatformText = "GroundPlatform_CACHED: ";
	bool bIsFalling = GetCharacterMovement()->IsFalling();
	//DrawDebugBox(GetWorld(), FootLocation, FVector(5, 5, 5), FColor::Black, false, .5, 56, 2);

	//GEngine->AddOnScreenDebugMessage(-1, 0, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("RotationArm: ") + *(RotationSpringArm->RelativeRotation.ToString())));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("CamForwardVector: ") + *CamForwardVector.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::FromHex("0090FFFF"), (TEXT("%s"), FString("GroundLocation: ") + *GroundLocation.ToString()));

	//Dimense Platform debug
	if (DimensePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), DimensePlatformText + DimensePlatform->GetName()));
	}
	else if (CachedDimensePlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("%s"), CachedDimensePlatformText + CachedDimensePlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), DimensePlatformText + FString("NULL")));
	}
	//Recent Ground Platform debug
	if (GroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), GroundPlatformText + GroundPlatform->GetName()));
	}
	else if (CachedGroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, (TEXT("%s"), CachedGroundPlatformText + CachedGroundPlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), GroundPlatformText + FString("NULL")));
	}
	//Move Around Edge Platform debug
	if (MoveAroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), MoveAroundPlatformText + MoveAroundPlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), MoveAroundPlatformText + FString("NULL")));
	}
	//Cached Move Around Edge Platform debug
	if (CachedMoveAroundPlatform) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), CachedMoveAroundPlatformText + CachedMoveAroundPlatform->GetName()));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), CachedMoveAroundPlatformText + FString("NULL")));
	}

	if (bSpinning) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("Spinning: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Spinning: false")));
	}
	//Can Move Around Edge debug
	if (bCanMoveAround) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("CanMoveAround: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("CanMoveAround: false")));
	}
	//Can Dimense debug
	if (bCanDimense) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("CanDimense: true")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("CanDimense: false")));
	}
	//Is Falling debug
	if (bIsFalling) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("OnGround: false")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("OnGround: true")));
	}
	//Cam Sign debug
	if (CamSign == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString("CamSign: ") + FString::FromInt(CamSign)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString("CamSign: ") + FString::FromInt(CamSign)));
	}
	//Cam Side debug
	if (CamSide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString("CamSide: ") + FString::FromInt(CamSide)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString("CamSide: ") + FString::FromInt(CamSide)));
	}
	//Visibility Side debug
	if (VisibilitySide == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("%s"), FString("Visibility: ") + FString::FromInt(VisibilitySide)));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, (TEXT("%s"), FString("Visibility: ") + FString::FromInt(VisibilitySide)));
	}
	//Movement Direction debug
	if (MovementDirection == 1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Moving Right")));
	}
	else if (MovementDirection == -1) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, (TEXT("Moving Left")));
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, (TEXT("Not Moving")));
	}
}