// Fill out your copyright notice in the Description page of Project Settings.


#include "Entities/EntityAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Components/CharacterStateComponent.h"
#include "Entities/Entity.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/Weapon/MeleeWeapon.h"
#include "Kismet/KismetMathLibrary.h"

void UEntityAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SetAnimationState(nullptr);
	
	EntityOwner = Cast<AEntity>(TryGetPawnOwner());
	OwnerUtils = EntityOwner;
	CharacterMovementProvider = EntityOwner;
	CharacterStateProvider = EntityOwner;
	
	if (CharacterMovementProvider)
	{
		MaxWalkSpeed = CharacterMovementProvider->GetMaxWalkSpeed();
	}
}

void UEntityAnimInstance::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (CharacterMovementProvider)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(CharacterMovementProvider->GetVelocity());
		IsFalling = OwnerUtils->IsFalling();

		bHasAcceleration = CharacterMovementProvider->GetCurrentAcceleration().SizeSquared2D() > SMALL_NUMBER;

		CharacterWeaponState = CharacterStateProvider->GetCurrentCharacterState().WeaponState;
		CharacterMode = CharacterStateProvider->GetCurrentCharacterState().Mode;
		
		bIsLocking = OwnerUtils->IsLocking();
		
		Direction = UKismetAnimationLibrary::CalculateDirection(CharacterMovementProvider->GetVelocity(),EntityOwner->GetActorRotation());
	}
}

void UEntityAnimInstance::SetAnimationState(const TScriptInterface<IWeaponInterface> WeaponEquipped)
{
	if (WeaponEquipped)
	{
		if (TScriptInterface<IMeleeWeapon> IsMeleeWeapon = WeaponEquipped.GetObject())
		{
			/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Is melee weapon");*/
			bIsMeleeWeapon = true;
		}
		else
		{
			/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, "Is not a melee weapon");*/
			bIsMeleeWeapon = false;
		}
	}
	else bIsMeleeWeapon = true;
}
