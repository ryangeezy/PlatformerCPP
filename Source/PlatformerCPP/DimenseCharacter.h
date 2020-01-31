// Copyright Ryan Gourley 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DimenseCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWorld;
class APlatformMaster;
class UParticleSystem;

UCLASS()
class PLATFORMERCPP_API ADimenseCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	//Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
		float MyHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
		float MyWidth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
		FVector FootLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
		FVector HeadLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		FVector CamForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int32 CamSide;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int32 CamSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int32 VisibilitySide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		float CameraZoomTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		float DefaultSpringArmLength;	

	//Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugPlatformRemote;

	//Functions
	UFUNCTION(BlueprintCallable, Category = "Movement") 
		FVector GetDimenseOffset(APlatformMaster* Platform, bool bDebugLocal) const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
		FVector GetMoveAroundOffset(FVector Location) const;

	UFUNCTION(Category = "Debug", meta = (BlueprintInternalUseOnly = "true"))
		void Debug() const;

	//Event Functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void ResetCanMoveAround() const; //ignore green squigly
		virtual void ResetCanMoveAround_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void StartCanMoveAroundTimer() const; //ignore green squigly
		virtual void StartCanMoveAroundTimer_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void ResetCanDimense() const; //ignore green squigly
		virtual void ResetCanDimense_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void StartCanDimenseTimer() const; //ignore green squigly
		virtual void StartCanDimenseTimer_Implementation() const;

private:
	//Default Required
	// Sets default values for this character's properties
	ADimenseCharacter();
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Variables
	int32 FacingDirection;
	int32 MovementDirection;
	float CachedJumpKeyHoldTime;
	FCollisionQueryParams QParams;
	FLatentActionInfo RotationSpringArmLatentInfo;
	FName TraceTag;
	FVector FromCameraLineVector;
	FVector CachedComponentVelocity;
	FVector CachedCharacterMovementVelocity;
	FVector GroundLocation;
	FVector LandingOffsetPadding;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult MoveAroundHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult DimenseHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult GroundHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult HeadHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult FrontHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult RearHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult LeftHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		FHitResult RightHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* GroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* MoveAroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedGroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedMoveAroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* DimensePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedDimensePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		FVector CamRightVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int32 DeathDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int32 FromCameraLineLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bJumpPressed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bMoveDownPressed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bCanMoveAround;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bCanDimense;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bSpinning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float WalkAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float GroundTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float HeadTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float SideTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float MeshRotationTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float DimenseTraceZOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
		TArray<TEnumAsByte<EObjectTypeQuery>> PickupObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
		TArray<AActor*> PickupIgnoreActors;

	//Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* PhysicsComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* RotationSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* MainCameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* MainCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* AntiCameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class USceneComponent* AntiCameraSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* PnPCaptureSpringArm;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		UParticleSystem* DeathParticle;

	//Functions
	bool SingleTrace(FHitResult& HitResult, FVector& Start, FVector& End, FCollisionQueryParams& QParams); //UFUNCTION MACRO DOESNT WORK with FCollisionQueryParams
	void InitDebug();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector GetMovementInputVectorFRI();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector CalculateTotalMovementInput(FVector Vector);

	UFUNCTION(BlueprintCallable, Category = "Utility", meta = (AllowPrivateAccess = "true"))
		FVector RoundVector(FVector Vector) const;

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int32 MoveLeftRight(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void JumpDown();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void RotateCamera(float Rotation);

	UFUNCTION(Category = "Default", meta = (AllowPrivateAccess = "true", BlueprintInternalUseOnly = "true"))
		void UpdateTickMovementVariables();

	UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
		void Respawn();

	UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
		void Die();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool TryDimense();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool TryMoveAround(FHitResult& HitResult, FVector Offset = FVector(0.f));

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void DoLineTracesAndPlatformChecks();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void RotateMeshToMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void MoveAround();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void SetMovementDirection();

	UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
		bool SetPlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, UPARAM(ref) AActor*& HitActor, FColor DebugColor, bool bDebugLocal);
	
	UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
		void InvalidatePlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, bool bDebugLocal, FColor DebugColor = FColor::Red);

	UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
		void InvalidateCachedPlatform(UPARAM(ref) APlatformMaster*& CachedPlatform, bool bDebugLocal, FColor DebugColor = FColor::Red);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void Dimense();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void PauseMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void ResumeMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void FindVisibilitySide();

	UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
		bool CheckDeath();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool VisibilityCheck(FVector Start);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool VerticalBoxTrace(FVector Location, FHitResult& HitResult, float TraceLength, int32 UpOrDown, FString DebugPhrase, bool bDebugLocal);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool HorizontalBoxTrace();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool HorizontalHitCheck(FVector Location, FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LeftHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool RightHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool IsFallingDownward();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetGroundPlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetMoveAroundPlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LineTraceForMoveAroundHit(FHitResult &HitResult, FVector Offset);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool PlayerAbovePlatformCheck(APlatformMaster* Platform);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LineTraceForDimenseHit(float ZOffset);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetDimensePlatform();
};