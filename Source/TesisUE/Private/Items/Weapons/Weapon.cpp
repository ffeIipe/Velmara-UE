// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"

#include "Components/CharacterStateComponent.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/ControllerProvider.h"

void AWeapon::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);
	
	ControllerProvider = NewOwner;
	CharacterStateProvider = NewOwner;
	AnimatorProvider = NewOwner;
	
	if (CharacterStateProvider)
	{
		CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	}
	
	if (AnimatorProvider)
	{
		AttachMeshToSocket(AnimatorProvider->GetMeshComponent());
	}
	
	if (ControllerProvider)
	{
		SetInstigator(ControllerProvider->GetEntityController()->GetInstigator());
	}
	
	ItemState = EItemState::EIS_Equipped;
	EnableWeaponState(true);
	DisableCollision();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeapon::EnableWeaponState(const bool bEnable) const
{
	if (CharacterStateProvider)
	{
		const ECharacterWeaponStates NewState = bEnable ? ECharacterWeaponStates::ECWS_EquippedWeapon : ECharacterWeaponStates::ECWS_Unequipped;
		CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetWeaponState(NewState);
	}
}
