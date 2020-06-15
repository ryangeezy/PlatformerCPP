// Copyright 2020 Ryan Gourley

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
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
			float CameraZoomTime;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
			float DefaultSpringArmLength;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
			float MyHeight;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
			float MyWidth;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
			FVector CamForwardVector;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
			FVector FootLocation;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
			FVector HeadLocation;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
			int32 CamSide;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
			int32 CamSign;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
			int32 VisibilitySide;

		//Debug
			UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
				bool bDebug;

			UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
				bool bDebugPlatformRemote;

	//Functions
		UFUNCTION(BlueprintCallable, Category = "Movement") 
			FVector GetDimenseOffset(const APlatformMaster* Platform, const bool bDebugLocal) const;

		UFUNCTION(BlueprintCallable, Category = "Movement")
			FVector GetMoveAroundOffset(const FVector& Location) const;

		UFUNCTION(Category = "Debug", meta = (BlueprintInternalUseOnly = "true"))
			void Debug() const;

		//Event Functions
			UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
				void ResetCanDimense() const; //ignore green squigly
				virtual void ResetCanDimense_Implementation() const;

			UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
				void ResetCanMoveAround() const; //ignore green squigly
				virtual void ResetCanMoveAround_Implementation() const;

			UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
				void StartCanDimenseTimer() const; //ignore green squigly
				virtual void StartCanDimenseTimer_Implementation() const;

			UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
				void StartCanMoveAroundTimer() const; //ignore green squigly
				virtual void StartCanMoveAroundTimer_Implementation() const;

private:
	//Default Required
		ADimenseCharacter(); // Sets default values for this character's properties		
		virtual void BeginPlay() override; // Called when the game starts or when spawned		
		virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override; // Called to bind functionality to input		
		virtual void Tick(float DeltaTime) override; // Called every frame

	//Variables
		float CachedJumpKeyHoldTime;
		FCollisionQueryParams QParams;
		FLatentActionInfo RotationSpringArmLatentInfo;
		FName TraceTag;
		FVector CachedCharacterMovementVelocity;
		FVector CachedComponentVelocity;
		FVector FromCameraLineVector;
		FVector GroundLocation;
		FVector LandingOffsetPadding;
		int32 FacingDirection;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* CachedDimensePlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* CachedMoveAroundPlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* CachedGroundPlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* DimensePlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* GroundPlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			APlatformMaster* MoveAroundPlatform = nullptr;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool bCanDimense;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool bCanMoveAround;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool bJumpPressed;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool bMoveDownPressed;		

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool bSpinning;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult DimenseHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult FrontHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult GroundHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult HeadHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult LeftHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult MoveAroundHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult RearHitResult;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			FHitResult RightHitResult;	

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float DimenseTraceZOffset;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float GroundTraceLength;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float HeadTraceLength;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float MeshRotationTime;

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float SideTraceLength;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			float WalkAcceleration;		

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
			FVector CamRightVector;

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			int32 DeathDistance;

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			int32 FromCameraLineLength;

		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Platform", meta = (AllowPrivateAccess = "true"))
			int32 MovementDirection;		

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
			TArray<AActor*> PickupIgnoreActors;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
			TArray<TEnumAsByte<EObjectTypeQuery>> PickupObjectTypes;

		//Components
			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				UCameraComponent* MainCamera;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
				UCapsuleComponent* PhysicsComp;

			UPROPERTY(EditDefaultsOnly, Category = "Default")
				UParticleSystem* DeathParticle;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				USceneComponent* AntiCameraSceneComponent;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				USpringArmComponent* AntiCameraSpringArm;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				USpringArmComponent* MainCameraSpringArm;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				USpringArmComponent* PnPCaptureSpringArm;

			UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
				USpringArmComponent* RotationSpringArm;

	//Functions
		void InitDebug();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool BoxTraceForDimenseHit(const float& ZOffset);

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool BoxTraceForMoveAroundHit(FHitResult& HitResult);

		UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
			bool CheckDeathByFallDistance();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool BoxTraceHorizontal(FHitResult& HitResult, const float& TraceLength, const int32 UpOrDown, const FString DebugPhrase, const bool bDebugLocal);

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool HorizontalHitCheck(const FVector& Location, FHitResult& HitResult);

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool IsFallingDownward() const;

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool LeftHitCheck();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool PlayerAbovePlatformCheck(const APlatformMaster* Platform, const bool bDebugLocal) const;

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool RightHitCheck();

		UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
			bool SetPlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, UPARAM(ref) FHitResult& HitResult, const FColor DebugColor, const bool bDebugLocal);
		
		/*
		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool SingleBoxTrace(UPARAM(ref) FHitResult& HitResult, const FVector& Start, const FVector& End, const FCollisionShape& Box) const;
		*/

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool SingleTrace(UPARAM(ref) FHitResult& HitResult, const FVector& Start, const FVector& End) const;

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool TryDimense();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool TryMoveAround(UPARAM(ref) FHitResult& HitResult);

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool BoxTraceVertical(const FVector& Location, FHitResult& HitResult, const float BoxHalfHeight, const float& TraceLength, const int32 UpOrDown, const FString DebugPhrase, const bool bDebugLocal);
		
		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			bool VisibilityCheck(const FVector& Start);
	
		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			FVector GetMovementInputFRI();

		UFUNCTION(BlueprintCallable, Category = "Utility", meta = (AllowPrivateAccess = "true"))
			FVector RoundVector(const FVector& Vector) const;

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			int32 MoveLeftRight(const float& AxisValue);

		UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
			void Die();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void Dimense();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void DoLineTracesAndPlatformChecks();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void SetMovementDirection();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void SetVisibilitySide();

		UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
			void InvalidateCachedPlatform(UPARAM(ref) APlatformMaster*& CachedPlatform, const bool bDebugLocal, const FColor DebugColor = FColor::Red);

		UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
			void InvalidatePlatform(UPARAM(ref) APlatformMaster*& Platform, UPARAM(ref) APlatformMaster*& CachedPlatform, const bool bDebugLocal, const FColor DebugColor = FColor::Red);
		
		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void JumpDown();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void MoveAround();		

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void PauseMovement();

		UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
			void Respawn();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void ResumeMovement();

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void RotateCamera(const float& Rotation);

		UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
			void RotateMeshToMovement();

		UFUNCTION(Category = "Default", meta = (AllowPrivateAccess = "true", BlueprintInternalUseOnly = "true"))
			void UpdateMovementSystemVariables();
};