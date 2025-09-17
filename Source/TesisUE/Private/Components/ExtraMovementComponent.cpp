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

void UExtraMovementComponent::InitializeValues(const FMovementData& MovementData)
{
	BufferCurve = MovementData.DodgeCurve;
	AnotherBufferCurve = MovementData.DodgeCurve;
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

	if (FOnTimelineFloat ProgressDodgeFunction; !ProgressDodgeFunction.IsBound() && BufferCurve)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, "Progress function bound... " + GetOwner()->GetName());
		
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(BufferCurve, ProgressDodgeFunction);
	}

	if (FOnTimelineFloat ProgressDodgeFunction; !ProgressDodgeFunction.IsBound() && AnotherBufferCurve)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, "Progress function bound... " + GetOwner()->GetName());
		
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(AnotherBufferCurve, ProgressDodgeFunction);
	}
}

void UExtraMovementComponent::PerformDoubleJump(UAnimMontage* DoubleJumpMontage)
{
	AnimatorProvider->PlayAnimMontage(DoubleJumpMontage);
	CharacterMovementProvider->LaunchCharacter(FVector(0.f, 0.f, DoubleJumpStrength), false, true);
	CanDoubleJump = false;
}

void UExtraMovementComponent::PerformDodge(const float DodgeDistance, UAnimMontage* DodgeAnim)
{
	if (AnotherBufferCurve)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, "Buffer Dodge VALID!");
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, "NULLPTR! Buffer Dodge");
	}
	
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
		
		BufferDodgeDistance = DodgeDistance;
		
		DodgeBufferEvent();
	
		PlayDodgeAnim(DodgeAnim);
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Dodge);
		
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString::SanitizeFloat(DodgeDistance));
	}
}

void UExtraMovementComponent::PlayDodgeAnim(UAnimMontage* DodgeMontage) const
{
	if (!CharacterMovementProvider->IsUsingControllerRotationYaw())
	{
		AnimatorProvider->PlayAnimMontage(DodgeMontage);
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

	AnimatorProvider->PlayAnimMontage(DodgeMontage, 1.f, Section);
}

void UExtraMovementComponent::DodgeSaveEvent()
{
	if (bIsSaveDodge)
	{
		bIsSaveDodge = false;

		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);

		if (OnDodgeSaved.IsBound())
		{
			OnDodgeSaved.Broadcast();
		}
	}
}

void UExtraMovementComponent::DodgeBufferEvent() const
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
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, "Buffer Dodge stopped.");
		BufferDodgeDistance = 0.f;
		BufferDodgeTimeline->Stop();
	}
}

void UExtraMovementComponent::UpdateDodgeBuffer(const float Alpha)
{
	if (bIsMoving)
	{
		const FVector CurrentLocation = GetOwner()->GetActorLocation();
		const FVector ForwardVector = CharacterMovementProvider->GetLastMovementInputVector();
		const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDodgeDistance), Alpha);

		GetOwner()->SetActorLocation(TargetLocation, true);
	}
	else
	{
		StopDodgeBufferEvent();
	}
}

void UExtraMovementComponent::PerformMove(const FVector2D& MoveVector, const bool bIsTriggered)
{
	CurrentMoveVector = MoveVector;
	bIsMoving = true;
	
	const FRotator ControlRotation = CharacterMovementProvider->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	CharacterMovementProvider->AddMovementInput(DirectionForward, MoveVector.Y);
	CharacterMovementProvider->AddMovementInput(DirectionSideward, MoveVector.X);

	if (!bIsTriggered)
	{
		CurrentMoveVector = FVector2D::ZeroVector;
		bIsMoving = false;
	}
}

void UExtraMovementComponent::PerformLook(const FVector2D& LookingVector) const
{
	CharacterMovementProvider->AddControllerPitchInput(LookingVector.Y);
	CharacterMovementProvider->AddControllerYawInput(LookingVector.X);
}
