#include "Components/ExtraMovementComponent.h"

#include "Entities/Entity.h"

#include "Components/CharacterStateComponent.h"
#include "Interfaces/AnimatorProvider.h"


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UExtraMovementComponent::IsMovingBackward() const
{
	if (CurrentMoveVector.Y < -.5f) //threshold
		return true;
	
	return false;
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AnimatorProvider = GetOwner();
	CharacterStateProvider = GetOwner();
	CharacterMovementProvider = GetOwner();
}

void UExtraMovementComponent::PerformDoubleJump(UAnimMontage* DoubleJumpMontage)
{
	AnimatorProvider->Execute_PlayAnimMontage(GetOwner(), DoubleJumpMontage, 1.f, "Default");
	CharacterMovementProvider->Execute_GetCharacter(GetOwner())->LaunchCharacter(FVector(0.f, 0.f, DoubleJumpForce), false, true);
	CanDoubleJump = false;
}

void UExtraMovementComponent::PlayDodgeAnim(UAnimMontage* DodgeMontage) const
{
	if (!CharacterMovementProvider->Execute_GetCharacter(GetOwner())->bUseControllerRotationYaw)
	{
		AnimatorProvider->Execute_PlayAnimMontage(GetOwner(), DodgeMontage, 1.f, "Default");
	}
	else
	{
		DodgeAnimBasedOnInput(DodgeMontage);	
	}
}

void UExtraMovementComponent::DodgeAnimBasedOnInput(UAnimMontage* DodgeMontage) const
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

	AnimatorProvider->Execute_PlayAnimMontage(GetOwner(), DodgeMontage, 1.f, Section);
}

void UExtraMovementComponent::DodgeSaveEvent()
{
	if (bIsSaveDodge)
	{
		bIsSaveDodge = false;

		CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetAction(ECharacterActionsStates::ECAS_Nothing);

		if (OnDodgeSaved.IsBound())
		{
			OnDodgeSaved.Broadcast();
		}
	}
}

void UExtraMovementComponent::PerformMove(const FVector2D& MoveVector, const bool bIsTriggered)
{
	CurrentMoveVector = MoveVector;
	bIsMoving = true;
	
	const FRotator ControlRotation = CharacterMovementProvider->Execute_GetCharacter(GetOwner())->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	CharacterMovementProvider->Execute_GetCharacter(GetOwner())->AddMovementInput(DirectionForward, MoveVector.Y);
	CharacterMovementProvider->Execute_GetCharacter(GetOwner())->AddMovementInput(DirectionSideward, MoveVector.X);

	if (!bIsTriggered)
	{
		CurrentMoveVector = FVector2D::ZeroVector;
		bIsMoving = false;
	}
}

void UExtraMovementComponent::PerformLook(const FVector2D& LookingVector) const
{
	CharacterMovementProvider->Execute_GetCharacter(GetOwner())->AddControllerPitchInput(LookingVector.Y);
	CharacterMovementProvider->Execute_GetCharacter(GetOwner())->AddControllerYawInput(LookingVector.X);
}