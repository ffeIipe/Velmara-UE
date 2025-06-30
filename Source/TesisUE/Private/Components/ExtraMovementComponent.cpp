#include "Components/ExtraMovementComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CharacterStateComponent.h"
#include <Components/CombatComponent.h>


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter>(GetOwner());
	OwnerCharacterStateComponent = GetOwner()->GetComponentByClass<UCharacterStateComponent>();

	if (BufferCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(BufferCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::Input_Dodge()
{
	if (OwningCharacter->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling ||
		OwningCharacter->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying) return;

	if (OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Dodge }))
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

		OwnerCharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		PerformDodge();
	}
}

void UExtraMovementComponent::PerformDodge()
{
	if (!OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Finish, ECharacterActions::ECA_Stun }))
	{
		if (GetOwner()->GetComponentByClass<UCombatComponent>())
		{
			GetOwner()->GetComponentByClass<UCombatComponent>()->RemoveSoftLockTarget();
		}

		FVector MovementInput = OwningCharacter->GetLastMovementInputVector();
		if (!MovementInput.IsNearlyZero())
		{
			FRotator LookRotation = MovementInput.Rotation();
			GetOwner()->SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
		}

		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		OwnerCharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Dodge);

		if (OwnerCharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
		{
			OwningCharacter->PlayAnimMontage(DodgeMontage);
		}
		else
		{
			OwningCharacter->GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;
			OwningCharacter->PlayAnimMontage(SpectralDodgeMontage);
		}
	}
}

void UExtraMovementComponent::DodgeBufferEvent(float BufferAmount)
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->PlayFromStart();
	}
}

void UExtraMovementComponent::StopDodgeBufferEvent()
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->Stop();
	}
}

void UExtraMovementComponent::UpdateDodgeBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferDodgeDistance);
}

void UExtraMovementComponent::UpdateBuffer(float Alpha, float BufferDistance)
{
	FVector CurrentLocation = GetOwner()->GetActorLocation();
	FVector ForwardVector = GetOwner()->GetActorForwardVector();

	FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UExtraMovementComponent::Input_DoubleJump()
{
	if (!OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead }))
	{
		OwningCharacter->PlayAnimMontage(DoubleJumpMontage);
		OwningCharacter->LaunchCharacter(FVector(0.f, 0.f, LaunchStrenght), false, true);
		CanDoubleJump = false;
	}
}
