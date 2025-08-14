#include "Components/ExtraMovementComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CharacterStateComponent.h"
#include <Components/CombatComponent.h>
#include <Entities/Entity.h>
#include "InputAction.h"
#include "DataAssets/EntityData.h"


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
}

void UExtraMovementComponent::InitializeValues(const FMovementData& MovementData)
{
	LaunchStrength = MovementData.LaunchStrength;
	MaxRunSpeed = MovementData.MaxRunSpeed;
	MaxStrafeSpeed = MovementData.MaxStrafeSpeed;
	BufferDodgeDistance = MovementData.BufferDodgeDistance;
	DodgeCurve = MovementData.DodgeCurve;
}

void UExtraMovementComponent::CustomInitialize(AEntity* NewEntity, UCharacterStateComponent* NewOwnerCharStateComp)
{
	EntityOwner = NewEntity;
	OwnerCharacterStateComponent = NewOwnerCharStateComp;
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	EntityOwner = Cast<AEntity>(GetOwner());
	DefaultWalkSpeed = EntityOwner->GetCharacterMovement()->MaxWalkSpeed;
	OwnerCharacterStateComponent = EntityOwner ? EntityOwner->GetCharacterStateComponent() : nullptr;
	

	if (DodgeCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(DodgeCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::Input_Dodge()
{
	if (EntityOwner->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling || 
		EntityOwner->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying ||
		EntityOwner->IsEquipping()) return;

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
		EntityOwner->GetCombatComponent()->RemoveSoftLockTarget();

		if (const FVector MovementInput = EntityOwner->GetLastMovementInputVector(); !MovementInput.IsNearlyZero())
		{
			const FRotator LookRotation = MovementInput.Rotation();
			GetOwner()->SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
		}

		if (!EntityOwner->bUseControllerRotationYaw)
		{
			EntityOwner->PlayAnimMontage(DodgeMontage, 1.f, "Default");
		}
		else
		{
			DodgeAnimBasedOnInput();	
		}

		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		OwnerCharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Dodge);
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
	EntityOwner->PlayAnimMontage(DodgeMontage, 1.f, Section);
}

void UExtraMovementComponent::DodgeBufferEvent(float BufferAmount)
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

void UExtraMovementComponent::UpdateBuffer(const float Alpha, const float BufferDistance)
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	const FVector ForwardVector = EntityOwner->GetLastMovementInputVector();

	const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UExtraMovementComponent::Input_Move(const FInputActionValue& Value)
{
	if (!OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun }))
	{
		MoveVector = Value.Get<FVector2D>();

		const FRotator ControlRotation = EntityOwner->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		EntityOwner->AddMovementInput(DirectionForward, MoveVector.Y);
		EntityOwner->AddMovementInput(DirectionSideward, MoveVector.X);

		UpdateAllowRunStrafe();
	}
}

void UExtraMovementComponent::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	EntityOwner->AddControllerPitchInput(LookingVector.Y);
	EntityOwner->AddControllerYawInput(LookingVector.X);
}

void UExtraMovementComponent::Input_Run(const FInputActionValue& Value)
{
	if (OwnerCharacterStateComponent->IsActionEqualToAny({
		ECharacterActions::ECA_Block,
		ECharacterActions::ECA_Finish,
		ECharacterActions::ECA_Dead,
		ECharacterActions::ECA_Stun })) return;

	if (EntityOwner->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling ||
		EntityOwner->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying) return;

	if (!Value.Get<bool>())
	{
		EntityOwner->GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
		return;
	}
		
	if (bAllowRun)
	{
		EntityOwner->GetCharacterMovement()->MaxWalkSpeed = MaxRunSpeed;

		if (bAllowRunStrafe)
		{
			EntityOwner->GetCharacterMovement()->MaxWalkSpeed = MaxStrafeSpeed;
		}
	}
	else EntityOwner->GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
}

void UExtraMovementComponent::UpdateAllowRunStrafe()
{
	MoveVector.Y > KINDA_SMALL_NUMBER ? bAllowRun = true : bAllowRun = false;

	MoveVector.X != 0.f ? bAllowRunStrafe = true : bAllowRunStrafe = false;
}

void UExtraMovementComponent::Input_DoubleJump()
{
	if (!OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead }))
	{
		EntityOwner->PlayAnimMontage(DoubleJumpMontage);
		EntityOwner->LaunchCharacter(FVector(0.f, 0.f, LaunchStrength), false, true);
		CanDoubleJump = false;
	}
}

void UExtraMovementComponent::PlayTurnInPlaceMontage(const FVector& DesiredInputDirection)
{
	const FVector CharacterForward = EntityOwner->GetActorForwardVector();
	const FVector NormalizedDesiredInputDirection = DesiredInputDirection.GetSafeNormal();

	const double CosAngle = FVector::DotProduct(CharacterForward, NormalizedDesiredInputDirection);
	double Angle = FMath::Acos(CosAngle);
	Angle = FMath::RadiansToDegrees(Angle);

	if (const FVector CrossProduct = FVector::CrossProduct(CharacterForward, NormalizedDesiredInputDirection); CrossProduct.Z < 0)
	{
		Angle *= -1.f;
	}

	FName SectionToPlay = FName("Default");

	if (Angle >= -45.f && Angle < 45.f)
	{
		return;
	}
	else if (Angle >= 45.f && Angle < 135.f)
	{
		SectionToPlay = FName("TurnRight");
	}
	else if (Angle >= -135.f && Angle < -45.f)
	{
		SectionToPlay = FName("TurnLeft");
	}
	else
	{
		SectionToPlay = FName("Turn180");
	}

	EntityOwner->PlayAnimMontage(TurnInPlaceMontage, 1.f, SectionToPlay);
}

//void UExtraMovementComponent::OnTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
//{
//	bIsTurningInPlace = false;
//}