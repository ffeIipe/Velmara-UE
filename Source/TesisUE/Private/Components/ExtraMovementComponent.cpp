#include "Components/ExtraMovementComponent.h"


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UExtraMovementComponent::IsMovingBackwards() const
{
	if (CurrentMoveVector.Y < -.7f) //threshold
		return true;
	
	return false;
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

FName UExtraMovementComponent::DodgeAnimBasedOnInput(UAnimMontage* DodgeMontage) const
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

	return Section;
}

void UExtraMovementComponent::DodgeSaveEvent()
{
	if (bIsSaveDodge)
	{
		bIsSaveDodge = false;

		if (OnDodgeSaved.IsBound())
		{
			OnDodgeSaved.Broadcast();
		}
	}
}