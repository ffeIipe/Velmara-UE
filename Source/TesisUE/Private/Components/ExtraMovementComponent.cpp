#include "Components/ExtraMovementComponent.h"

#include <Entities/Entity.h>

#include "InputAction.h"
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
	DoubleJumpStrength = MovementData.LaunchStrength;
	BufferDodgeDistance = MovementData.BufferDodgeDistance;
	DodgeCurve = MovementData.DodgeCurve;
}

void UExtraMovementComponent::CustomInitialize(AEntity* NewEntity)
{
	OwnerUtils = NewEntity;
	CharacterStateProvider = NewEntity;
}

bool UExtraMovementComponent::IsMovingBackwards() const
{
	if (MoveVector.Y < 0)
		return true;

	return false;
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerUtils = GetOwner();
	AnimatorProvider = GetOwner();
	CharacterStateProvider = GetOwner();
	
	if (DodgeCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(DodgeCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::Input_Dodge()
{
	if (OwnerUtils->IsFalling() || 
		OwnerUtils->IsFlying() /*||
		EntityOwner->IsEquipping()*/) return;

	if (CharacterStateProvider->IsActionEqualToAny({ ECharacterActions::ECA_Dodge }))
	{
		bIsSaveDodge = true;
	}
	else
	{
		PerformDodge();
	}
}

void UExtraMovementComponent::DodgeSaveEvent()
{
	if (bIsSaveDodge)
	{
		bIsSaveDodge = false;

		CharacterStateProvider->SetAction(ECharacterActions::ECA_Nothing);
		PerformDodge();
	}
}

void UExtraMovementComponent::PerformDodge()
{
	if (!CharacterStateProvider->IsActionEqualToAny({ ECharacterActions::ECA_Finish, ECharacterActions::ECA_Stun }))
	{
		if (OnDodgeStarted.IsBound()) OnDodgeStarted.Broadcast();

		if (const FVector MovementInput = CharacterMovementProvider->GetLastMovementInputVector(); !MovementInput.IsNearlyZero())
		{
			const FRotator LookRotation = MovementInput.Rotation();
			GetOwner()->SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
		}

		if (!CharacterMovementProvider->IsUsingControllerRotationYaw())
		{
			AnimatorProvider->PlayAnimMontage(DodgeMontage);
		}
		else
		{
			DodgeAnimBasedOnInput();	
		}

		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		CharacterStateProvider->SetAction(ECharacterActions::ECA_Dodge);
	}
}

void UExtraMovementComponent::DodgeAnimBasedOnInput() const
{
	FName Section("Default");

	if (MoveVector.Y == 0.f)
	{
		if (MoveVector.X > KINDA_SMALL_NUMBER)
		{
			Section = FName("DodgeRight");
		}
		else
		{
			Section = FName("DodgeLeft");
		}
	}
	else if (MoveVector.Y < 0.f)
	{
		Section = FName("DodgeBack");
	}

	AnimatorProvider->PlayAnimMontage(DodgeMontage, 1.f, Section);
}

void UExtraMovementComponent::DodgeBufferEvent(float BufferAmount) const
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

void UExtraMovementComponent::UpdateDodgeBuffer(const float Alpha)
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

void UExtraMovementComponent::Input_Move(const FInputActionValue& Value)
{
	if (!CharacterStateProvider->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun }))
	{
		MoveVector = Value.Get<FVector2D>();

		const FRotator ControlRotation = CharacterMovementProvider->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		CharacterMovementProvider->AddMovementInput(DirectionForward, MoveVector.Y);
		CharacterMovementProvider->AddMovementInput(DirectionSideward, MoveVector.X);
	}
}

void UExtraMovementComponent::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	CharacterMovementProvider->AddControllerPitchInput(LookingVector.Y);
	CharacterMovementProvider->AddControllerYawInput(LookingVector.X);
}

void UExtraMovementComponent::Input_DoubleJump()
{
	if (!CharacterStateProvider->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead }))
	{
		AnimatorProvider->PlayAnimMontage(DoubleJumpMontage);
		CharacterMovementProvider->LaunchCharacter(FVector(0.f, 0.f, DoubleJumpStrength), false, true);
		CanDoubleJump = false;
	}
}