// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BufferComponent.h"

#include "Components/TimelineComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/ControllerProvider.h"


UBufferComponent::UBufferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferTimelineComp = CreateDefaultSubobject<UTimelineComponent>(TEXT("Buffer Timeline"));
}

void UBufferComponent::StartLocationBuffer(const float Distance, UCurveFloat* Curve, const bool bIsCameraForwardVectorUsed)
{
	if (Curve && Curve != CurrentCurve)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f, FColor::Blue, "Curve loaded.");
		CurrentCurve = Curve;
		FOnTimelineFloat TimelineEventFunction;
		TimelineEventFunction.BindUFunction(this, "UpdateLocationBuffer");
		BufferTimelineComp->AddInterpFloat(Curve,TimelineEventFunction);	
	}

	bIsCameraForwardVector = bIsCameraForwardVectorUsed;

	if (bIsCameraForwardVectorUsed)
	{
		if (const TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider = GetOwner())
		{
			CharacterMovementProvider->Execute_GetCharacterMovementComponent(GetOwner())->SetMovementMode(MOVE_Flying);
			
			FOnTimelineEvent TimelineEvent;
			TimelineEvent.BindUFunction(this, "SetDefaultMovement");
			BufferTimelineComp->SetTimelineFinishedFunc(TimelineEvent);
		}
	}
	
	CurrentDistance = 0.f;
	CurrentDistance = Distance;

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Distance: " + FString::SanitizeFloat(CurrentDistance));
	
	BufferTimelineComp->Stop();
	BufferTimelineComp->PlayFromStart();
}

void UBufferComponent::StopLocationBuffer() const
{
	if (BufferTimelineComp)
		BufferTimelineComp->Stop();
}

/*void UBufferComponent::StartRotationBuffer(const TObjectPtr<UCurveFloat>& Curve)
{
}

void UBufferComponent::StopRotationBuffer() const
{
}*/

void UBufferComponent::UpdateLocationBuffer(float Alpha)
{
	if (const TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider = GetOwner();
		CharacterMovementProvider->Execute_GetExtraMovementComponent(GetOwner())->IsMoving())
	{
		const FVector CurrentLocation = GetOwner()->GetActorLocation();

		FVector ForwardVector; 
		
		if (!bIsCameraForwardVector)
		{
			ForwardVector = CharacterMovementProvider->Execute_GetCharacter(GetOwner())->GetLastMovementInputVector();
		}
		else
		{
			if (const TScriptInterface<IControllerProvider> ControllerProvider = GetOwner())
			{
				FRotator CurrentRotation;
				ControllerProvider->GetEntityController()->GetPlayerViewPoint(ForwardVector, CurrentRotation);
				ForwardVector = CurrentRotation.Vector();
			}
			else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Failed to get Controller Provider.");
		}
		
		const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * CurrentDistance) * BufferMultiplier, Alpha);

		GetOwner()->SetActorLocation(TargetLocation, true);
	}
	else
	{
		StopLocationBuffer();
	}

	
}

void UBufferComponent::SetDefaultMovement()
{
	if (const TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider = GetOwner())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, "Set Default Movement called.");
		CharacterMovementProvider->Execute_GetCharacterMovementComponent(GetOwner())->SetMovementMode(MOVE_Falling);
	}
}

/*void UBufferComponent::UpdateRotationBuffer(float Alpha)
{
}*/
