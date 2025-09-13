#include "Components/ExtraMovementComponent.h"

#include <Entities/Entity.h>

#include "Components/CharacterStateComponent.h"
#include "Components/TimelineComponent.h"
#include "DataAssets/EntityData.h"
#include "Interfaces/AnimatorProvider.h"


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
}

void UExtraMovementComponent::InitializeValues(const FMovementData& MovementData)
{
	DodgeCurve = MovementData.DodgeCurve;
}

void UExtraMovementComponent::SetCurrentStrategyValues(const float DodgeDistance, const float DoubleJumpForce, UAnimMontage* NewDodgeMontage)
{
	BufferDodgeDistance = DodgeDistance;
	DoubleJumpStrength = DoubleJumpForce; 
	CurrentDodgeMontage = NewDodgeMontage;
}

void UExtraMovementComponent::CustomInitialize(AEntity* NewEntity)
{
	OwnerUtils = NewEntity;
	CharacterStateProvider = NewEntity;
}

bool UExtraMovementComponent::IsMovingBackwards() const
{
	if (CurrentMoveVector.Y < -.5f) //threshold
		return true;
	
	return false;
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerUtils = GetOwner();
	AnimatorProvider = GetOwner();
	CharacterStateProvider = GetOwner();
	CharacterMovementProvider = GetOwner();
	StrategyProvider = GetOwner();
	
	if (DodgeCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(DodgeCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::PerformDoubleJump()
{
	AnimatorProvider->PlayAnimMontage(DoubleJumpMontage);
	CharacterMovementProvider->LaunchCharacter(FVector(0.f, 0.f, DoubleJumpStrength), false, true);
	CanDoubleJump = false;
}

void UExtraMovementComponent::PerformDodge()
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ECharacterActionsStates::ECAS_Dodge}))
	{
		bIsSaveDodge = true;
	}
	else
	{
		if (OnDodgeStarted.IsBound()) OnDodgeStarted.Broadcast();

		if (const FVector MovementInput = CharacterMovementProvider->GetLastMovementInputVector(); !MovementInput.IsNearlyZero())
		{
			const FRotator LookRotation = MovementInput.Rotation();
			GetOwner()->SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
		}

		PlayDodgeAnim();
		
		StopDodgeBufferEvent();
		DodgeBufferEvent();
	
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Dodge);
	}
}

void UExtraMovementComponent::PlayDodgeAnim() const
{
	if (!CharacterMovementProvider->IsUsingControllerRotationYaw())
	{
		AnimatorProvider->PlayAnimMontage(CurrentDodgeMontage);
	}
	else
	{
		DodgeAnimBasedOnInput();	
	}
}

void UExtraMovementComponent::DodgeAnimBasedOnInput() const
{
	FName Section("Default");

	if (CurrentMoveVector.Y == 0.f)
	{
		if (CurrentMoveVector.X > KINDA_SMALL_NUMBER)
		{
			Section = FName("DodgeRight");
		}
		else
		{
			Section = FName("DodgeLeft");
		}
	}
	else if (CurrentMoveVector.Y < 0.f)
	{
		Section = FName("DodgeBack");
	}

	AnimatorProvider->PlayAnimMontage(CurrentDodgeMontage, 1.f, Section);
}

void UExtraMovementComponent::DodgeSaveEvent()
{
	if (bIsSaveDodge)
	{
		bIsSaveDodge = false;

		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
		PerformDodge();
	}
}

void UExtraMovementComponent::DodgeBufferEvent() const
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->PlayFromStart();
	}
}

void UExtraMovementComponent::StopDodgeBufferEvent() const
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->Stop();
	}
}

void UExtraMovementComponent::UpdateDodgeBuffer(const float Alpha) const
{
	UpdateBuffer(Alpha, BufferDodgeDistance);
}

void UExtraMovementComponent::UpdateBuffer(const float Alpha, const float BufferDistance) const
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	const FVector ForwardVector = CharacterMovementProvider->GetLastMovementInputVector();

	const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UExtraMovementComponent::PerformMove(const FVector2D& MoveVector, const bool bIsTriggered)
{
	CurrentMoveVector = MoveVector;
	
	const FRotator ControlRotation = CharacterMovementProvider->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	CharacterMovementProvider->AddMovementInput(DirectionForward, MoveVector.Y);
	CharacterMovementProvider->AddMovementInput(DirectionSideward, MoveVector.X);

	if (!bIsTriggered)
	{
		CurrentMoveVector = FVector2D::ZeroVector;
	}
}

void UExtraMovementComponent::PerformLook(const FVector2D& LookingVector) const
{
	CharacterMovementProvider->AddControllerPitchInput(LookingVector.Y);
	CharacterMovementProvider->AddControllerYawInput(LookingVector.X);
}
