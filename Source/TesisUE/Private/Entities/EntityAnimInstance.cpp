// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/EntityAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Components/CharacterStateComponent.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Kismet/KismetMathLibrary.h"

void UEntityAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Owner = TryGetPawnOwner();
	OwnerUtils = Owner;
	CharacterMovementProvider = Owner;
	CharacterStateProvider = Owner;
}

void UEntityAnimInstance::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (CharacterMovementProvider)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(CharacterMovementProvider->GetVelocity());
		IsFalling = OwnerUtils->IsFalling();

		bHasAcceleration = CharacterMovementProvider->GetCurrentAcceleration().SizeSquared2D() > SMALL_NUMBER;

		CharacterHumanState = CharacterStateProvider->GetCurrentCharacterState().HumanState;
		CharacterMode = CharacterStateProvider->GetCurrentCharacterState().Mode;
		SpectralState = CharacterStateProvider->GetCurrentCharacterState().SpectralState;
		
		bIsLocking = OwnerUtils->IsLocking();
		
		Direction = UKismetAnimationLibrary::CalculateDirection(CharacterMovementProvider->GetVelocity(),Owner->GetActorRotation());
	}
}
