#include "Components/ExtraMovementComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CharacterStateComponent.h"
#include <Components/CombatComponent.h>
#include <Entities/Entity.h>
#include "InputAction.h"


UExtraMovementComponent::UExtraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
}

void UExtraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	EntityOwner = Cast<AEntity>(GetOwner());

	OwningCharacter = Cast<ACharacter>(GetOwner());
	OwnerCharacterStateComponent = EntityOwner->GetCharacterStateComponent();

	if (BufferCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(BufferCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::Input_Dodge()
{
	if (EntityOwner->GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Falling || EntityOwner->GetCharacterMovement()->GetGroundMovementMode() == EMovementMode::MOVE_Flying || !EntityOwner->IsEquipping())

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

void UExtraMovementComponent::Input_Move(const FInputActionValue& Value)
{
	if (!OwnerCharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun }))
	{
		const FVector2D MoveVector = Value.Get<FVector2D>();

		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Purple, FString::SanitizeFloat(EntityOwner->GetCharacterMovement()->GetMaxSpeed()));

		const FVector CurrentCharacterForward = EntityOwner->GetActorForwardVector();
		const FVector CurrentCharacterRight = EntityOwner->GetActorRightVector();
		const FVector DesiredInputDirection = (CurrentCharacterForward * MoveVector.Y + CurrentCharacterRight * MoveVector.X).GetSafeNormal();

		LastInputDirection = DesiredInputDirection;

		if (EntityOwner->GetCharacterMovement()->Velocity.SizeSquared() < KINDA_SMALL_NUMBER &&
			!DesiredInputDirection.IsNearlyZero())
		{
			PlayTurnInPlaceMontage(DesiredInputDirection);
		}*/

		const FRotator ControlRotation = EntityOwner->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		EntityOwner->AddMovementInput(DirectionForward, MoveVector.Y);
		EntityOwner->AddMovementInput(DirectionSideward, MoveVector.X);
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
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::White, FString("Enter Input_Run"));
	if (Value.Get<bool>())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Cyan, FString("True pressing Input_Run"));

		EntityOwner->GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Purple, FString("False Input_Run"));

		EntityOwner->GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
	}
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

void UExtraMovementComponent::PlayTurnInPlaceMontage(const FVector& DesiredInputDirection)
{
	const FVector CharacterForward = OwningCharacter->GetActorForwardVector();
	const FVector NormalizedDesiredInputDirection = DesiredInputDirection.GetSafeNormal();

	const double CosAngle = FVector::DotProduct(CharacterForward, NormalizedDesiredInputDirection);
	double Angle = FMath::Acos(CosAngle);
	Angle = FMath::RadiansToDegrees(Angle);

	const FVector CrossProduct = FVector::CrossProduct(CharacterForward, NormalizedDesiredInputDirection);
	if (CrossProduct.Z < 0)
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

	OwningCharacter->PlayAnimMontage(TurnInPlaceMontage, 1.f, SectionToPlay);
}

//void UExtraMovementComponent::OnTurnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
//{
//	bIsTurningInPlace = false;
//}