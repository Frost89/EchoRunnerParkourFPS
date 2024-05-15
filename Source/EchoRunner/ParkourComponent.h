// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class ACharacter;
class UCharacterMovementComponent;
class UCameraShakeBase;

UENUM(BlueprintType)
enum class EParkourMode : uint8
{
	NONE	UMETA(DisplayName = "None"),
	LEFTWALLRUN		UMETA(DisplayName = "LeftWallRun"),
	RIGHTWALLRUN	UMETA(DisplayName = "RightWallRun"),
	VERTICALWALLRUN		UMETA(DisplayName = "VerticalWallRun"),
	LEDGEGRAB	UMETA(DisplayName = "LedgeGrab"),
	MANTLE	UMETA(DisplayName = "Mantle"),
	SLIDE	UMETA(DisplayName = "Slide"),
	SPRINT	UMETA(DisplayName = "Sprint"),
	CROUCH UMETA(DisplayName = "Crouch")
};

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Components/ActorComponent.h"
#include "ParkourComponent.generated.h"


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHORUNNER_API UParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UParkourComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void JumpEvent();
	UFUNCTION(BlueprintCallable)
	void LandEvent();
	UFUNCTION(BlueprintCallable)
	void DashEvent();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void UpdateEvent();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenWallRunGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseWallRunGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenVerticalWallRunGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseVerticalWallRunGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void VerticalWallRunEndEvent();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenMantleGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseMantleGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenCheckMantleGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseCheckMantleGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenSlideGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseSlideGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OpenSprintGate();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CloseSprintGate();

	UFUNCTION(BlueprintCallable)
	void Initialise(ACharacter* Char);
	
	//WallRunFunctions
	UFUNCTION(BlueprintCallable)
	bool WallRunMovement(FVector Start, FVector End, float WallRunDir);
	UFUNCTION(BlueprintCallable)
	void WallRunUpdate();
	UFUNCTION(BlueprintCallable)
	void WallRunEnableGravity();
	UFUNCTION(BlueprintCallable)
	void CorrectWallRunLocation();
	UFUNCTION(BlueprintCallable)
	void ApplyGravityAndCorrectLocation();
	UFUNCTION(BlueprintCallable)
	void WallRunJump();
	UFUNCTION(BlueprintCallable)
	void WallRunEnd(float ResetTime);

	//VerticalWallRunFunctions
	UFUNCTION(BlueprintCallable)
	void VerticalWallRunMovement();
	UFUNCTION(BlueprintCallable)
	void VerticalWallRunUpdate();
	UFUNCTION(BlueprintCallable)
	void VerticalWallRunEnd(float ResetTime);
	UFUNCTION(BlueprintCallable)
	void CorrectVerticalWallRunLocation();
	UFUNCTION(BlueprintCallable)
	void CorrectLedgeLocation();
	UFUNCTION(BlueprintCallable)
	void ForwardTracer(FHitResult& OutHit, bool& ValidHit);

	//MantleFunctions
	UFUNCTION(BlueprintCallable)
	void MantleCheck();
	UFUNCTION(BlueprintCallable)
	void MantleStart();
	UFUNCTION(BlueprintCallable)
	void MantleMovement();
	UFUNCTION(BlueprintCallable)
	void LedgeGrabJump();

	//SlideFunctions
	UFUNCTION(BlueprintCallable)
	void SlideUpdate();
	UFUNCTION(BlueprintCallable)
	void SlideStart();
	UFUNCTION(BlueprintCallable)
	void SlideEnd(bool bCrouch);
	UFUNCTION(BlueprintCallable)
	void SlideJump();

	//SprintFunctions
	UFUNCTION(BlueprintCallable)
	void SprintUpdate();
	UFUNCTION(BlueprintCallable)
	void SprintStart();
	UFUNCTION(BlueprintCallable)
	void SprintEnd();
	UFUNCTION(BlueprintCallable)
	void SprintJump();
	UFUNCTION(BlueprintCallable)
	void SprintEvent();

	//CrouchFunctions
	UFUNCTION(BlueprintCallable)
	void CrouchStart();
	UFUNCTION(BlueprintCallable)
	void CrouchEnd();
	UFUNCTION(BlueprintCallable)
	void CrouchJump();
	UFUNCTION(BlueprintCallable)
	void CrouchSlideEvent();
	UFUNCTION(BlueprintCallable)
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable)
	void JumpMovement();

	//ManipulateGatesAndQueues
	UFUNCTION(BlueprintCallable)
	void OpenGates();
	UFUNCTION(BlueprintCallable)
	void CloseGates();
	UFUNCTION(BlueprintCallable)
	void CheckQueues();
	UFUNCTION(BlueprintCallable)
	void JumpEvents();
	UFUNCTION(BlueprintCallable)
	void EndEvents();

	//ParkourMovementFunctions
	UFUNCTION(BlueprintCallable)
	void MovementChanged(EMovementMode PrevMovement, EMovementMode CurrentMovement);
	UFUNCTION(BlueprintCallable)
	void ParkourChanged(EParkourMode PrevParkour, EParkourMode CurrentParkour);
	UFUNCTION(BlueprintCallable)
	bool SetParkourMode(EParkourMode NewMode);
	UFUNCTION(BlueprintCallable)
	void ResetMovement();

	//CameraFunctions
	UFUNCTION(BlueprintCallable)
	void PlayCameraShake(TSubclassOf<UCameraShakeBase> Shake);
	UFUNCTION(BlueprintCallable)
	void CameraTilt(float TargetRoll);
	UFUNCTION(BlueprintCallable)
	void CameraTick();
	UFUNCTION(BlueprintCallable)
	void UpdateCameraProperties();

	//CameraShakeClasses
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Shakes")
	TSubclassOf<UCameraShakeBase> JumpLand;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Shakes")
	TSubclassOf<UCameraShakeBase> LedgeGrab;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Shakes")
	TSubclassOf<UCameraShakeBase> Mantle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera Shakes")
	TSubclassOf<UCameraShakeBase> QuickMantle;

	//CheckOrGetterFunctions
	FVector GetWallRunEndVector(float LineTraceRange);
	FVector GetWallRunTargetVector();
	FRotator GetWallRunTargetRotation();
	FVector GetVerticalWallRunTargetVector();
	FRotator GetVerticalWallRunTargetRotation();
	FVector GetLedgeTargetVector();
	FRotator GetLedgeTargetRotation();
	FVector GetDashLaunchVelocity();

	void GetMantleVectors(FVector& OutEyes, FVector& OutFeet);
	float InterpolateGravity();

	UFUNCTION(BlueprintPure)
	bool IsWallRunning();
	float ForwardInput();
	bool CanWallRun();
	bool CanMantle();
	bool CanQuickMantle();
	bool CanVerticalWallRun();
	bool CanSprint();
	bool CanSlide();
	void GrabLedge();
	bool LedgeMantleOrVertical();
	bool CanJump();

	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	ACharacter* Character;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	UCharacterMovementComponent* CharacterMovement;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	float DefaultGravity;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	float DefaultGroundFriction;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	float DefaultBrakingDeceleration;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	float DefaultWalkSpeed;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	float DefaultCrouchSpeed;
	UPROPERTY(BlueprintReadOnly, Category = "Character Properties")
	bool bDefaultUseControllerRotationYaw;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Dash")
	bool bCanDash;

	UPROPERTY(EditDefaultsOnly, Category = "Wall Running")
	float WallRunSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "Wall Running")
	float WallRunSprintSpeed;
	UPROPERTY(EditDefaultsOnly, Category = "Wall Running")
	float LedgeGrabJumpOffForce;
	UPROPERTY(EditDefaultsOnly, Category = "Wall Running")
	float LedgeGrabJumpOffHeight;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Wall Running")
	float VerticalWallRunTime;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Wall Running")
	float MantleHeight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Running")
	bool OnWall;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprint")
	bool bAlwaysSprint;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprint")
	float SprintSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide")
	float SlideImpulseAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashScale = 15.0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashRange = 2000.0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float MaxRangeScale = 100.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	int32 TimesJumped;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	int32 MaxJumps;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour and Movement")
	TEnumAsByte<EMovementMode> PrevMovementMode;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour and Movement")
	TEnumAsByte<EMovementMode> CurrentMovementMode;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour and Movement")
	EParkourMode PrevParkourMode;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Parkour and Movement")
	EParkourMode CurrentParkourMode;

private:
	FVector WallRunNormal;
	FVector WallRunLocation;

	FVector VerticalWallRunNormal;
	FVector VerticalWallRunLocation;
	float VerticalWallRunSpeed;
	float VerticalWallRunCurrentSpeed;

	bool bSprintQueued;
	bool bSlideQueued;

	FVector GetSlideVector();

	FVector MantlePosition;
	float MantleTraceDistance;
	float MantleSpeed;
	float QuickMantleSpeed;

	FVector LedgeFloorPosition;
	FVector LedgeClimbWallNormal;
	FVector LedgeClimbWallPosition;
	bool bLedgeCloseToGround;

	bool bCameraShake;

	UPROPERTY(EditAnywhere, Category = "Wall Running")
	bool WallRunGravityOn;
	UPROPERTY(EditAnywhere, Category = "Wall Running")
	float WallJumpScale;
	UPROPERTY(EditAnywhere, Category = "Wall Running")
	float WallJumpForce;
	UPROPERTY(EditAnywhere, Category = "Wall Running")
	float WallRunTargetGravity;

};
