// Copyright Ryan Gourley 2019

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
	//Functions
	UFUNCTION(BlueprintCallable, Category = "Movement")
		FVector GetDimenseOffset(APlatformMaster* Platform, bool bDebugLocal);

	UFUNCTION(BlueprintCallable, Category = "Movement")
		FVector GetMoveAroundEdgeOffset(FVector Location);

	UFUNCTION(Category = "Debug", meta = (BlueprintInternalUseOnly = "true"))
		void Debug();

	//Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		FVector CamForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
		float MyHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
		float MyWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		float CameraZoomTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int CamSide;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int CamSign;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		int VisibilitySide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		float DefaultSpringArmLength;

	//Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugPlatformRemote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugSidesTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugVisibilityTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugDimenseTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bDebugMoveAroundEdge;

	//Events	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void ResetCanMoveAroundEdge();
	virtual void ResetCanMoveAroundEdge_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void StartCanMoveAroundEdgeTimer();
	virtual void StartCanMoveAroundEdgeTimer_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void ResetCanDimense();
	virtual void ResetCanDimense_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Timers")
		void StartCanDimenseTimer();
	virtual void StartCanDimenseTimer_Implementation();

private:
	// Sets default values for this character's properties
	ADimenseCharacter();
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Functions
	UFUNCTION(Category = "Default", meta = (AllowPrivateAccess = "true", BlueprintInternalUseOnly = "true"))
		void UpdateTickMovementVariables();

	UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
		bool CheckDeath();

	UFUNCTION(BlueprintCallable, Category = "Events", meta = (AllowPrivateAccess = "true"))
		void Respawn();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool VisibilityCheck(FVector Start);

	//UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))   -MACRO DOESNT WORK with FCollisionQueryParams
		bool SingleTrace(FHitResult &HitResult, FVector &Start, FVector &End, FCollisionQueryParams &QParams);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool GroundHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool HeadHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LeftHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool RightHitCheck();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void TryMoveAroundEdge(FHitResult &HitResult, FVector Offset = FVector(0.f));

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool IsFallingDownward();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void DoLineTracesAndPlatformChecks();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void RotateMeshToMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void MoveAroundEdge();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetRecentGroundPlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetMoveAroundEdgePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void InvalidateMoveAroundEdgePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void InvalidateCachedMoveAroundEdgePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LineTraceForMoveAroundEdgeHit(FHitResult &HitResult, FVector Offset);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void InvalidateDimensePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void TryDimense();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool FindDimensePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool FindMoveAroundEdgePlatform(FHitResult &HitResult, FVector Offset);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void FindVisibilitySide();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool PlayerAbovePlatformCheck(APlatformMaster* Platform);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool LineTraceForDimenseHit(float ZOffset);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool SetDimensePlatform();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void Dimense();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void PauseMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void ResumeMovement();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int MoveLeftRight(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void JumpDown();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector GetMovementVectorFRI();

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector CalculateTotalMovementInput(FVector Vector);

	UFUNCTION(BlueprintCallable, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		void RotateCamera(float Rotation);

	UFUNCTION(BlueprintCallable, Category = "Platforms", meta = (AllowPrivateAccess = "true"))
		void InvalidateRecentGroundPlatform();

	UFUNCTION(BlueprintCallable, Category = "Utility", meta = (AllowPrivateAccess = "true"))
		FVector RoundVector(FVector Vector);

	//Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
		FVector CamRightVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult MoveAroundEdgeHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult DimenseHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult GroundHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult HeadHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult FrontHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult RearHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult LeftHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FHitResult RightHitResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FName DimenseTraceTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FName SidesTraceTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FName VisibilityTraceTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitResult", meta = (AllowPrivateAccess = "true"))
		FName MoveAroundEdgeTraceTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bJumpPressed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bMoveDownPressed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bCanMoveAroundEdge;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bCanDimense;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		bool bSpinning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float WalkAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float GroundTraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float MeshRotationTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float GroundAndHeadTraceRadiusMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float DimenseTraceZOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		UCapsuleComponent* PhysicsComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* RecentGroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* DimensePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* MoveAroundEdgePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedGroundPlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedDimensePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		APlatformMaster* CachedMoveAroundEdgePlatform = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int FacingDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int DeathDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int FromCameraLineLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		int MovementDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float SideTraceLength;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float DefaultJumpZVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		float CachedJumpKeyHoldTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector FromCameraLineVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector CachedComponentVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector CachedCharacterMovementVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector RecentGroundLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
		FVector LandingOffsetPadding;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
		TArray<TEnumAsByte<EObjectTypeQuery>> PickupObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickups", meta = (AllowPrivateAccess = "true"))
		TArray<AActor*> PickupIgnoreActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (AllowPrivateAccess = "true"))
		FVector FootLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (AllowPrivateAccess = "true"))
		FVector HeadLocation;

	FCollisionShape PlayerCollisionShape;
	FCollisionQueryParams DimenseQParams;
	FCollisionQueryParams SidesQParams;
	FCollisionQueryParams VisibilityQParams;
	FCollisionQueryParams MoveAroundQParams;
	FLatentActionInfo RotationSpringArmLatentInfo;

	//Components
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
};