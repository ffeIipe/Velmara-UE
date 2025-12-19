// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"

#include "Components/CharacterStateComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/ControllerProvider.h"

void AWeapon::Pick(AActor* NewOwner)
{
	/*ControllerProvider = NewOwner;
	CharacterStateProvider = NewOwner;
	AnimatorProvider = NewOwner;
	
	if (CharacterStateProvider != nullptr)
	{
		if (CharacterStateProvider->Execute_GetCharacterStateComponent(NewOwner)->IsModeEqualToAny({ECharacterModeStates::ECMS_Spectral}))
		{
			return;
		}
	}*/
	
	UCharacterStateComponent* CharacterStateComponent = NewOwner->FindComponentByClass<UCharacterStateComponent>();
	USkeletalMeshComponent* Mesh = Cast<ACharacter>(NewOwner)->GetMesh();
	APawn* NewInstigator = Cast<APawn>(NewOwner);

	Super::Pick(NewOwner);
	
	if (CharacterStateComponent)
	{
		CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	}
	
	if (Mesh)
	{
		AttachMeshToSocket(Mesh);
	}
	
	if (NewInstigator)
	{
		SetInstigator(NewInstigator);
	}
	
	ItemState = EItemState::EIS_Equipped;
	EnableWeaponState(true);
	DisableCollision();
}

void AWeapon::EnableWeaponState(const bool bEnable) const
{
	if (CharacterStateProvider)
	{
		const ECharacterWeaponStates NewState = bEnable ? ECharacterWeaponStates::ECWS_EquippedWeapon : ECharacterWeaponStates::ECWS_Unequipped;
		CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetWeaponState(NewState);
	}
}