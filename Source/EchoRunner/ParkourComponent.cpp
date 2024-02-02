// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraShakeBase.h"
#include "ParkourComponent.h"

// Sets default values for this component's properties
UParkourComponent::UParkourComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	OnWall = false;
	WallRunGravityOn = true;
	WallRunSpeed = 850.0;
	WallRunSprintSpeed = 1100.0;
	WallRunTargetGravity = 0.2;
	WallJumpForce = 600.0;
	WallJumpScale = 600.0;
	bCameraShake = true;
	bAlwaysSprint = true;
	MantleHeight = 44.0;
	MantleSpeed = 10.0;
	QuickMantleSpeed = 20.0;
	VerticalWallRunSpeed = 300;
	VerticalWallRunCurrentSpeed = VerticalWallRunSpeed;
	LedgeGrabJumpOffForce = 300.0;
	LedgeGrabJumpOffHeight = 400.0;
	SprintSpeed = 1000.0;
	VerticalWallRunTime = 1.0;
	SlideImpulseAmount = 600.0;
	// ...
}


// Called when the game starts
void UParkourComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UParkourComponent::JumpEvent()
{
	if (CurrentParkourMode == EParkourMode::NONE)
	{
		if (CharacterMovement->IsFalling() == false)
		{
			OpenGates();
			PlayCameraShake(JumpLand);
		}
	}
	else
	{
		JumpEvents();
	}
}

void UParkourComponent::LandEvent()
{
	EndEvents();
	CloseGates();
	PlayCameraShake(JumpLand);
}

void UParkourComponent::Initialise(ACharacter* Char)
{
	Character = Char;
	CharacterMovement = Character->GetCharacterMovement();
	DefaultGravity = CharacterMovement->GravityScale;
	DefaultGroundFriction = CharacterMovement->GroundFriction;
	DefaultBrakingDeceleration = CharacterMovement->BrakingDecelerationWalking;
	DefaultWalkSpeed = CharacterMovement->MaxWalkSpeed;
	DefaultCrouchSpeed = CharacterMovement->MaxWalkSpeedCrouched;
	UpdateCameraProperties();

	FTimerDelegate TimerDelegate;
	FTimerHandle TimerHandle;
	TimerDelegate.BindUFunction(this, FName("UpdateEvent"));
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 0.0167, true);
}

bool UParkourComponent::WallRunMovement(FVector Start, FVector End, float WallRunDir)
{
	FHitResult wallhit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(
		wallhit,
		Start,
		End,
		ECC_Visibility
	);
	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, true, 1.0);
	if (wallhit.bBlockingHit)
	{
		//UE_LOG(LogTemp, Warning, TEXT("WallHit: True"));

		WallRunNormal = wallhit.Normal;
		WallRunLocation = wallhit.ImpactPoint;

		if (WallRunNormal.Z > -0.52 && WallRunNormal.Z < 0.52 && CharacterMovement->IsFalling())
		{
			//UE_LOG(LogTemp, Warning, TEXT("In WallRange"));

			//Push player forward or backward
			FVector CrossProdWallRunNormal = FVector::CrossProduct(WallRunNormal, FVector(0, 0, 1));
			float Speed = (bSprintQueued) ? WallRunSprintSpeed : WallRunSpeed;
			float WallRunVectorScale = WallRunDir * Speed;
			FVector FwdBwdLaunchVel = CrossProdWallRunNormal * WallRunVectorScale;
			bool bZOverride = (!IsWallRunning() or !WallRunGravityOn);
			Character->LaunchCharacter(FwdBwdLaunchVel, true, bZOverride);
			OnWall = true;
			//UE_LOG(LogTemp, Warning, TEXT("OnWall"),OnWall);
			return OnWall;
		}
		else
		{
			OnWall = false;
			return OnWall;
		}
	}
	else
	{
		OnWall = false;
		return OnWall;
	}
}

void UParkourComponent::WallRunUpdate()
{
	if (CanWallRun())
	{
		//RightSideWallRun
		bool isOnWallR = WallRunMovement(Character->GetActorLocation(), GetWallRunEndVector(75.0), -1.0);
		if (isOnWallR)
		{
			bool changed = SetParkourMode(EParkourMode::RIGHTWALLRUN);
			if (changed)
			{
				ApplyGravityAndCorrectLocation();
			}
			else
			{
				CharacterMovement->GravityScale = InterpolateGravity();
			}
		}
		else
		{
			if (CurrentParkourMode == EParkourMode::RIGHTWALLRUN)
			{
				WallRunEnd(0.5);
			}
			else
			{
				bool isOnWallL = WallRunMovement(Character->GetActorLocation(), GetWallRunEndVector(-75.0), 1.0);
				if (isOnWallL)
				{
					bool changed = SetParkourMode(EParkourMode::LEFTWALLRUN);
					if (changed)
					{
						ApplyGravityAndCorrectLocation();
					}
					else
					{
						CharacterMovement->GravityScale = InterpolateGravity();
					}
				}
				else
				{
					WallRunEnd(0.5);
				}
			}
		}
	}
	else
	{
		WallRunEnd(1.0);
	}
}

void UParkourComponent::WallRunEnableGravity()
{
	WallRunGravityOn = IsWallRunning();
}

void UParkourComponent::CorrectWallRunLocation()
{
	if (IsWallRunning())
	{
		FLatentActionInfo LAInfo;
		LAInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo(
			Character->GetCapsuleComponent(),
			GetWallRunTargetVector(),
			GetWallRunTargetRotation(),
			false,
			false,
			0.1,
			false,
			EMoveComponentAction::Move,
			LAInfo);
	}
}

void UParkourComponent::ApplyGravityAndCorrectLocation()
{
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;
	TimerDel.BindUFunction(this, FName("WallRunEnableGravity"));
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 1.0, false);
	CorrectWallRunLocation();
	CharacterMovement->GravityScale = InterpolateGravity();
}

void UParkourComponent::WallRunJump()
{
	//UE_LOG(LogTemp, Warning, TEXT("C++ Jump called"));
	if (IsWallRunning())
	{
		WallRunEnd(0.35);
		//Launch Character
		float XOverride = WallJumpScale * WallRunNormal.X;
		float YOverride = WallJumpScale * WallRunNormal.Y;
		UE_LOG(LogTemp, Warning, TEXT("Launching Character"));
		Character->LaunchCharacter(FVector(XOverride, YOverride, WallJumpForce), false, true);
	}
}

void UParkourComponent::WallRunEnd(float ResetTime)
{
	if (IsWallRunning())
	{
		//UE_LOG(LogTemp, Warning, TEXT("WallRunEnd called"));
		bool change = SetParkourMode(EParkourMode::NONE);
		if (change)
		{
			CloseWallRunGate();
			
			FTimerDelegate TimerDelegate;
			FTimerHandle TimerHandle;
			TimerDelegate.BindUFunction(this, FName("OpenWallRunGate"));
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, ResetTime, false);
			
			FTimerDelegate GTimerDel;
			FTimerHandle GTimerHandle;
			GTimerDel.BindUFunction(this, FName("WallRunEnableGravity"));
			GetWorld()->GetTimerManager().SetTimer(GTimerHandle, GTimerDel, 0.0, false); 
			WallRunGravityOn = false;
		}
	}
}

void UParkourComponent::VerticalWallRunMovement()
{
	FHitResult FwdTracerHit;
	bool bTracerValidHit;
	ForwardTracer(FwdTracerHit, bTracerValidHit);
	if (bTracerValidHit)
	{
		VerticalWallRunLocation = FwdTracerHit.Location;
		VerticalWallRunNormal = FwdTracerHit.Normal;
		bool changed = SetParkourMode(EParkourMode::VERTICALWALLRUN);
		if (changed)
		{
			CorrectVerticalWallRunLocation();
		}
		FVector VertWROverride = VerticalWallRunNormal * -600.0;
		Character->LaunchCharacter(FVector(VertWROverride.X, VertWROverride.Y, VerticalWallRunSpeed), true, true);
	}
	else
	{
		VerticalWallRunEnd(0.35);
	}
}

void UParkourComponent::VerticalWallRunUpdate()
{
	if (CanVerticalWallRun())
	{
		FVector Eyes, Feet;
		GetMantleVectors(Eyes, Feet);
		FHitResult OutHit;
		bool hit = GetWorld()->SweepSingleByChannel(OutHit,
			Eyes,
			Feet,
			Character->GetActorQuat(),
			ECC_Visibility,
			FCollisionShape::MakeCapsule(20.0, 10.0));
		if (hit)
		{
			MantleTraceDistance = OutHit.Distance;
			LedgeFloorPosition = OutHit.ImpactPoint;
			FHitResult FTOutHit;
			bool FTValidHit;
			ForwardTracer(FTOutHit, FTValidHit);
			if (CharacterMovement->IsWalkable(OutHit) and FTValidHit)
			{
				LedgeClimbWallPosition = FTOutHit.ImpactPoint;
				LedgeClimbWallNormal = FTOutHit.Normal;
				float CapHalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
				MantlePosition = LedgeFloorPosition + FVector(0.0, 0.0, CapHalfHeight*1.0);
				CloseVerticalWallRunGate();
				GrabLedge();

				FHitResult LedgeOutHit;
				float CapZOffset = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 40.0;
				FVector EndVec = Character->GetActorLocation() - (Character->GetActorUpVector() * CapZOffset);
				bLedgeCloseToGround = GetWorld()->LineTraceSingleByChannel(
					LedgeOutHit,
					Character->GetActorLocation(),
					EndVec,
					ECC_Visibility
				);

				if (CanQuickMantle())
				{
					OpenCheckMantleGate();
				}
				else
				{
					CorrectLedgeLocation();
					FTimerHandle TimerHandle;
					FTimerDelegate TimerDelegate;
					TimerDelegate.BindUFunction(this, FName("OpenCheckMantleGate"));
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 0.25, false);
				}
			}
			else
			{
				VerticalWallRunMovement();
			}
		}
		else
		{
			VerticalWallRunMovement();
		}
	}
	else
	{
		VerticalWallRunEnd(0.35);
	}
}

void UParkourComponent::VerticalWallRunEnd(float ResetTime)
{
	if (LedgeMantleOrVertical())
	{
		bool change = SetParkourMode(EParkourMode::NONE);
		if (change)
		{
			//VerticalWallRunCurrentSpeed = VerticalWallRunSpeed;
			CloseVerticalWallRunGate();
			CloseCheckMantleGate();
			bLedgeCloseToGround = false;

			FTimerDelegate TimerDelegate;
			FTimerHandle TimerHandle;
			TimerDelegate.BindUFunction(this, FName("OpenVerticalWallRunGate"));
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, ResetTime, false);

			FTimerDelegate QueueTimerDelegate;
			FTimerHandle QueueTimerHandle;
			QueueTimerDelegate.BindUFunction(this, FName("CheckQueues"));
			GetWorld()->GetTimerManager().SetTimer(QueueTimerHandle, QueueTimerDelegate, 0.02, false);
		}
	}
}

void UParkourComponent::CorrectVerticalWallRunLocation()
{
	if (CurrentParkourMode == EParkourMode::VERTICALWALLRUN)
	{
		FLatentActionInfo LAInfo;
		LAInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo(
			Character->GetCapsuleComponent(),
			GetVerticalWallRunTargetVector(),
			GetVerticalWallRunTargetRotation(),
			false,
			false,
			0.1,
			false,
			EMoveComponentAction::Move,
			LAInfo
		);
	}
}

void UParkourComponent::CorrectLedgeLocation()
{

	if (CurrentParkourMode == EParkourMode::LEDGEGRAB)
	{
		FLatentActionInfo LAInfo;
		LAInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo(
			Character->GetCapsuleComponent(),
			GetLedgeTargetVector(),
			GetLedgeTargetRotation(),
			false,
			false,
			0.1,
			false,
			EMoveComponentAction::Move,
			LAInfo
		);
	}
}

void UParkourComponent::ForwardTracer(FHitResult& OutHit, bool& ValidHit)
{
	FVector Eyes, Feet;
	GetMantleVectors(Eyes, Feet);
	FHitResult OutHitLocal;
	FVector EndVec = Feet + (Character->GetActorForwardVector() * 50.0);
	bool hit = GetWorld()->SweepSingleByChannel(
		OutHitLocal, 
		Feet,
		EndVec,
		Character->GetActorQuat(),
		ECC_Visibility,
		FCollisionShape::MakeCapsule(10.0, 5.0)
		);
	ValidHit = OutHitLocal.bBlockingHit and (OutHitLocal.Normal.Z >= -0.1);
	OutHit = OutHitLocal;
}

void UParkourComponent::MantleCheck()
{
	if (CanMantle())
	{
		MantleStart();
	}
}

void UParkourComponent::MantleStart()
{
	bool change = SetParkourMode(EParkourMode::MANTLE);
	if (change)
	{
		if (CanQuickMantle())
		{
			PlayCameraShake(QuickMantle);
		}
		else
		{
			PlayCameraShake(Mantle);
		}
		CloseCheckMantleGate();
		OpenMantleGate();
	}
}

void UParkourComponent::MantleMovement()
{
	//SetRotationToLookAtMantle
	FVector PlayerLocation = Character->GetActorLocation();
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
		FVector(PlayerLocation.X, PlayerLocation.Y, 0.0),
		FVector(MantlePosition.X, MantlePosition.Y, 0.0));
	FRotator NewRot = FMath::RInterpTo(Character->GetControlRotation(), LookAtRot, GetWorld()->GetDeltaSeconds(), 7.0);
	Character->GetController()->SetControlRotation(NewRot);

	//SetActorLocationAtMantlePosition
	float InterpSpeed = (CanQuickMantle()) ? QuickMantleSpeed : MantleSpeed;
	FVector NewLoc = FMath::VInterpTo(PlayerLocation, MantlePosition, GetWorld()->GetDeltaSeconds(), InterpSpeed);
	Character->SetActorLocation(NewLoc);

	if (FVector::Distance(Character->GetActorLocation(), MantlePosition) < 8.0)
	{
		VerticalWallRunEnd(0.5);
	}
}

void UParkourComponent::LedgeGrabJump()
{
	if (LedgeMantleOrVertical())
	{
		VerticalWallRunEnd(0.35);
		float XOverride = VerticalWallRunNormal.X * LedgeGrabJumpOffForce;
		float YOverride = VerticalWallRunNormal.Y * LedgeGrabJumpOffForce;
		Character->LaunchCharacter(FVector(XOverride, YOverride, LedgeGrabJumpOffHeight), false, true);
	}
}

void UParkourComponent::SlideUpdate()
{
	if (CurrentParkourMode == EParkourMode::SLIDE)
	{
		if (CharacterMovement->Velocity.Length() <= 35.0)
		{
			SlideEnd(false);
		}
	}
}

void UParkourComponent::SlideStart()
{
	if (CanSlide() and CharacterMovement->IsWalking())
	{
		SprintEnd();
		SetParkourMode(EParkourMode::SLIDE);
		Character->Crouch();
		CharacterMovement->GroundFriction = 0.0;
		CharacterMovement->BrakingDecelerationWalking = 1000.0;
		CharacterMovement->MaxWalkSpeedCrouched = 0.0;
		CharacterMovement->SetPlaneConstraintFromVectors(
			CharacterMovement->Velocity.GetSafeNormal(), 
			Character->GetActorUpVector());
		CharacterMovement->SetPlaneConstraintEnabled(true);
		FVector SlideVector = GetSlideVector();
		if (SlideVector.Z <= 0.02)
		{
			CharacterMovement->AddImpulse(SlideVector * SlideImpulseAmount, true);
		}
		OpenSlideGate();
		bSprintQueued = false;
		bSlideQueued = false;
	}
}

void UParkourComponent::SlideEnd(bool bCrouch)
{
	if (CurrentParkourMode == EParkourMode::SLIDE)
	{
		EParkourMode NewMode = bCrouch ? EParkourMode::CROUCH : EParkourMode::NONE;
		bool changed = SetParkourMode(NewMode);
		if (changed)
		{
			CloseSlideGate();
			if (bCrouch == false)
			{
				Character->UnCrouch();
			}
		}
	}
}

void UParkourComponent::SlideJump()
{
	if (CurrentParkourMode == EParkourMode::SLIDE)
	{
		SlideEnd(false);
	}
}

void UParkourComponent::SprintUpdate()
{
	if (CurrentParkourMode == EParkourMode::SPRINT)
	{
		if ((ForwardInput() > 0.0) == false)
		{
			SprintEnd();
		}
	}
}

void UParkourComponent::SprintStart()
{
	SlideEnd(false);
	CrouchEnd();

	if (CanSprint())
	{
		if (SetParkourMode(EParkourMode::SPRINT))
		{
			CharacterMovement->MaxWalkSpeed = SprintSpeed;
			OpenSprintGate();
			bSprintQueued = false;
			bSlideQueued = false;
		}
	}

}

void UParkourComponent::SprintEnd()
{
	if (CurrentParkourMode == EParkourMode::SPRINT)
	{
		bool changed = SetParkourMode(EParkourMode::NONE);
		if (changed)
		{
			CloseSprintGate();

			FTimerDelegate TimerDel;
			FTimerHandle TimerHandle;
			TimerDel.BindUFunction(this, FName("OpenSprintGate"));
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 0.1, false);
		}
	}
}

void UParkourComponent::SprintJump()
{
	if (CurrentParkourMode == EParkourMode::SPRINT)
	{
		SprintEnd();
		bSprintQueued = true;
	}
}

void UParkourComponent::SprintEvent()
{
	SprintStart();
}

void UParkourComponent::CrouchStart()
{
	if (CurrentParkourMode == EParkourMode::NONE)
	{
		Character->Crouch();
		SetParkourMode(EParkourMode::CROUCH);
		bSprintQueued = false;
		bSlideQueued = false;
	}
}

void UParkourComponent::CrouchEnd()
{
	if (CurrentParkourMode == EParkourMode::CROUCH)
	{
		Character->UnCrouch();
		SetParkourMode(EParkourMode::NONE);
		bSprintQueued = false;
		bSlideQueued = false;
	}
}

void UParkourComponent::CrouchJump()
{
	if (CurrentParkourMode == EParkourMode::CROUCH)
	{
		CrouchEnd();
	}
}

void UParkourComponent::CrouchSlideEvent()
{
	if (LedgeMantleOrVertical())
	{
		VerticalWallRunEnd(0.5);
	}
	else if (IsWallRunning())
	{
		WallRunEnd(0.5);
	}
	else
	{
		if (CanSlide())
		{
			if (CharacterMovement->IsWalking())
			{
				SlideStart();
			}
			else
			{
				bSlideQueued = true;
			}
		}
		else
		{
			ToggleCrouch();
		}
	}
}

void UParkourComponent::ToggleCrouch()
{
	switch (CurrentParkourMode)
	{
	case EParkourMode::NONE:
		CrouchStart();
		break;
	case EParkourMode::LEFTWALLRUN:
		break;
	case EParkourMode::RIGHTWALLRUN:
		break;
	case EParkourMode::VERTICALWALLRUN:
		break;
	case EParkourMode::LEDGEGRAB:
		break;
	case EParkourMode::MANTLE:
		break;
	case EParkourMode::SLIDE:
		break;
	case EParkourMode::SPRINT:
		break;
	case EParkourMode::CROUCH:
		CrouchEnd();
		break;
	default:
		break;
	}
}

void UParkourComponent::OpenGates()
{
	OpenWallRunGate();
	OpenVerticalWallRunGate();
	OpenSlideGate();
	OpenSprintGate();
}

void UParkourComponent::CloseGates()
{
	CloseWallRunGate();
	CloseVerticalWallRunGate();
	CloseSlideGate();
	CloseSprintGate();
}

void UParkourComponent::CheckQueues()
{
	if (bSlideQueued)
	{
		SlideStart();
	}
	else if (bSprintQueued)
	{
		SprintStart();
	}
}

void UParkourComponent::JumpEvents()
{
	WallRunJump();
	LedgeGrabJump();
	SlideJump();
	CrouchJump();
	SprintJump();
}

void UParkourComponent::EndEvents()
{
	WallRunEnd(0.0);
	VerticalWallRunEnd(0.0);
	SprintEnd();
	SlideEnd(false);

}

void UParkourComponent::MovementChanged(EMovementMode PrevMovement, EMovementMode CurrentMovement)
{
	if (Character)
	{
		PrevMovementMode = PrevMovement;
		CurrentMovementMode = CurrentMovement;
		if (PrevMovementMode == EMovementMode::MOVE_Walking and CurrentMovementMode == EMovementMode::MOVE_Falling)
		{
			SprintJump();
			EndEvents();
			OpenGates();
		}
		else
		{
			if (PrevMovementMode == EMovementMode::MOVE_Falling and CurrentMovementMode == EMovementMode::MOVE_Walking)
			{
				CheckQueues();
			}
		}
	}
}

void UParkourComponent::ParkourChanged(EParkourMode PrevParkour, EParkourMode CurrentParkour)
{
	PrevParkourMode = PrevParkour;
	CurrentParkourMode = CurrentParkour;
	ResetMovement();
}

bool UParkourComponent::SetParkourMode(EParkourMode NewMode)
{
	if (CurrentParkourMode == NewMode)
	{
		return false;
	}
	else
	{
		ParkourChanged(CurrentParkourMode, NewMode);
		return true;
	}
}

void UParkourComponent::ResetMovement()
{
	if (CurrentParkourMode == EParkourMode::NONE or CurrentParkourMode == EParkourMode::CROUCH)
	{
		CharacterMovement->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = bDefaultUseControllerRotationYaw;

		CharacterMovement->GravityScale = DefaultGravity;
		CharacterMovement->GroundFriction = DefaultGroundFriction;
		CharacterMovement->BrakingDecelerationWalking = DefaultBrakingDeceleration;
		CharacterMovement->MaxWalkSpeed = DefaultWalkSpeed;
		CharacterMovement->MaxWalkSpeedCrouched = DefaultCrouchSpeed;
		CharacterMovement->SetPlaneConstraintEnabled(false);

		switch (PrevParkourMode)
		{
		case EParkourMode::NONE:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			break;
		case EParkourMode::LEFTWALLRUN:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Falling);
			break;
		case EParkourMode::RIGHTWALLRUN:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Falling);
			break;
		case EParkourMode::VERTICALWALLRUN:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Falling);
			break;
		case EParkourMode::LEDGEGRAB:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Falling);
			break;
		case EParkourMode::MANTLE:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			break;
		case EParkourMode::SLIDE:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			break;
		case EParkourMode::SPRINT:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			break;
		case EParkourMode::CROUCH:
			CharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			break;
		default:
			break;
		}
	}
	else
	{
		CharacterMovement->bOrientRotationToMovement = (CurrentParkourMode == EParkourMode::SPRINT);
		Character->bUseControllerRotationYaw = (CurrentParkourMode == EParkourMode::SPRINT and bDefaultUseControllerRotationYaw);
	}
}

void UParkourComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> Shake)
{
	if (bCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), Shake, Character->GetActorLocation(), 0.0, 100.0);
	}
}

void UParkourComponent::CameraTilt(float TargetRoll)
{
	FRotator ControlRot = Character->GetController()->GetControlRotation();
	FRotator NewRot = FMath::RInterpTo(ControlRot, 
		FRotator(ControlRot.Pitch, ControlRot.Yaw, TargetRoll), 
		GetWorld()->GetDeltaSeconds(), 
		10.0);
	Character->GetController()->SetControlRotation(NewRot);
}

void UParkourComponent::CameraTick()
{
	switch (CurrentParkourMode)
	{
	case EParkourMode::NONE:
		CameraTilt(0.0);
		break;
	case EParkourMode::LEFTWALLRUN:
		CameraTilt(15.0);
		break;
	case EParkourMode::RIGHTWALLRUN:
		CameraTilt(-15.0);
		break;
	case EParkourMode::VERTICALWALLRUN:
		CameraTilt(0.0);
		break;
	case EParkourMode::LEDGEGRAB:
		CameraTilt(0.0);
		break;
	case EParkourMode::MANTLE:
		CameraTilt(0.0);
		break;
	case EParkourMode::SLIDE:
		CameraTilt(-15.0);
		break;
	case EParkourMode::SPRINT:
		CameraTilt(0.0);
		break;
	case EParkourMode::CROUCH:
		CameraTilt(0.0);
		break;
	default:
		break;
	}
}

void UParkourComponent::UpdateCameraProperties()
{
	bDefaultUseControllerRotationYaw = Character->bUseControllerRotationYaw;
}

FVector UParkourComponent::GetWallRunEndVector(float LineTraceRange)
{
	FVector CharLocation = Character->GetActorLocation();
	FVector RightVec = Character->GetActorRightVector();
	FVector FwdVector = Character->GetActorForwardVector();

	FVector SideLineTrace = RightVec * LineTraceRange;
	FVector FwdLineTrace = FwdVector * -35.0;

	FVector Endpoint = CharLocation + SideLineTrace + FwdLineTrace;
	return Endpoint;
}

FVector UParkourComponent::GetWallRunTargetVector()
{
	FVector NormalTimesCapsuleRadius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * WallRunNormal;
	return (NormalTimesCapsuleRadius + WallRunLocation);
}

FRotator UParkourComponent::GetWallRunTargetRotation()
{
	FRotator RelativeRot = Character->GetCapsuleComponent()->GetRelativeRotation();
	FRotator WallRunNormalXVecRot = UKismetMathLibrary::MakeRotFromX(WallRunNormal);
	float angle = (CurrentParkourMode == EParkourMode::LEFTWALLRUN) ? 90.0 : -90.0;
	float NewYaw = WallRunNormalXVecRot.Yaw - angle;
	return FRotator(RelativeRot.Pitch, NewYaw, RelativeRot.Roll);
}

FVector UParkourComponent::GetVerticalWallRunTargetVector()
{
	FVector NormalTimesCapsuleRadius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * VerticalWallRunNormal;
	return (NormalTimesCapsuleRadius + VerticalWallRunLocation);
}

FRotator UParkourComponent::GetVerticalWallRunTargetRotation()
{
	FRotator RelativeRot = Character->GetCapsuleComponent()->GetRelativeRotation();
	FRotator VerticalWallRunNormalXVecRot = UKismetMathLibrary::MakeRotFromX(VerticalWallRunNormal);
	float NewYaw = VerticalWallRunNormalXVecRot.Yaw - 180.0;
	return FRotator(RelativeRot.Pitch, NewYaw, RelativeRot.Roll);
}

FVector UParkourComponent::GetLedgeTargetVector()
{
	float CapRadius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	float CapHalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FVector NormalTimesCapsuleRadius = LedgeClimbWallNormal * CapRadius;
	FVector SumOfNormalAndPosition = NormalTimesCapsuleRadius + LedgeClimbWallPosition;
	float LedgeZFromCapsule = LedgeFloorPosition.Z - CapHalfHeight;

	return FVector(SumOfNormalAndPosition.X, SumOfNormalAndPosition.Y, LedgeZFromCapsule);
}

FRotator UParkourComponent::GetLedgeTargetRotation()
{
	FRotator RelativeRot = Character->GetCapsuleComponent()->GetRelativeRotation();
	FRotator LedgeNormalXVecRot = UKismetMathLibrary::MakeRotFromX(LedgeClimbWallNormal);
	float NewYaw = LedgeNormalXVecRot.Yaw - 180.0;
	return FRotator(RelativeRot.Pitch, NewYaw, RelativeRot.Roll);
}

void UParkourComponent::GetMantleVectors(FVector& OutEyes, FVector& OutFeet)
{
	//Eyes
	FVector EyesLocation;
	FRotator EyesRotation;
	Character->GetController()->GetActorEyesViewPoint(EyesLocation, EyesRotation);
	EyesLocation.Z = EyesLocation.Z + 50.0;
	FVector FwdVec = Character->GetActorForwardVector() * 50.0;
	OutEyes = EyesLocation + FwdVec;

	//Feet
	float CapHalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FVector FeetLocation = Character->GetActorLocation() - FVector(0, 0, (CapHalfHeight - MantleHeight));
	OutFeet = FeetLocation + FwdVec;
}


float UParkourComponent::InterpolateGravity()
{
	return FMath::FInterpTo(CharacterMovement->GravityScale, WallRunTargetGravity, GetWorld()->GetDeltaSeconds(), 20.0);
}

bool UParkourComponent::IsWallRunning()
{
	return (CurrentParkourMode == EParkourMode::LEFTWALLRUN or CurrentParkourMode == EParkourMode::RIGHTWALLRUN);
}

float UParkourComponent::ForwardInput()
{
	return FVector::DotProduct(Character->GetActorForwardVector(), CharacterMovement->GetLastInputVector());
}

bool UParkourComponent::CanWallRun()
{
	return ((CurrentParkourMode == EParkourMode::NONE or IsWallRunning()) and ForwardInput() > 0.0);
}

bool UParkourComponent::CanMantle()
{
	return ((ForwardInput() > 0.0) and (CurrentParkourMode == EParkourMode::LEDGEGRAB or CanQuickMantle()));
}

bool UParkourComponent::CanQuickMantle()
{
	return ((MantleTraceDistance > MantleHeight) or bLedgeCloseToGround);
}

bool UParkourComponent::CanVerticalWallRun()
{
	bool bViableParkourModes = (CurrentParkourMode == EParkourMode::NONE or CurrentParkourMode == EParkourMode::VERTICALWALLRUN
		or IsWallRunning());
	return ((ForwardInput() > 0.0) and CharacterMovement->IsFalling() and bViableParkourModes);
}

bool UParkourComponent::CanSprint()
{
	return (CharacterMovement->IsWalking() and CurrentParkourMode == EParkourMode::NONE);
}

bool UParkourComponent::CanSlide()
{
	return ((ForwardInput() > 0.0) and (CurrentParkourMode == EParkourMode::SPRINT or bSprintQueued));
}

void UParkourComponent::GrabLedge()
{
	bool change = SetParkourMode(EParkourMode::LEDGEGRAB);
	if (change)
	{
		CharacterMovement->DisableMovement();
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->GravityScale = 0.0;
		PlayCameraShake(LedgeGrab);
	}
}

bool UParkourComponent::LedgeMantleOrVertical()
{
	return (CurrentParkourMode == EParkourMode::LEDGEGRAB or CurrentParkourMode == EParkourMode::VERTICALWALLRUN or CurrentParkourMode == EParkourMode::MANTLE);
}

FVector UParkourComponent::GetSlideVector()
{
	FVector EndVec = Character->GetActorLocation() + (Character->GetActorUpVector() * -200.0);
	FHitResult OutHit;
	GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Character->GetActorLocation(),
		EndVec,
		ECC_Visibility
	);
	FVector CrossProduct = FVector::CrossProduct(OutHit.ImpactNormal, Character->GetActorRightVector());
	return (CrossProduct* -1.0);
}


// Called every frame
void UParkourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

