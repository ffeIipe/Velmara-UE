// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"

#include "LoadSystem/PlayerProgressSaveGame.h"
#include "SceneEvents/NewGameStateBase.h"
#include "Tutorial/PromptWidgetComponent.h"

void AWeapon::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);
	
	ControllerProvider = NewOwner;
	CharacterStateProvider = NewOwner;
	AnimatorProvider = NewOwner;
	
	DisableCollision();
	//TODO: Unify this\/\/\/
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (PromptWidget)
	{
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (!bWasUsed)
	{
		bWasUsed = true;
		if (const UWorld* World = GetWorld())
		{
			if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
			{
				FInteractedItemSaveData SaveData;
				SaveData.UniqueSaveID = GetUniqueSaveID();
				SaveData.bWasOpened = bWasUsed;
				GameState->UpdateInteractedItemState(SaveData);
			}
		}
	}
	
	if (CharacterStateProvider)
	{
		CharacterStateProvider->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
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
	EnableVisuals(true);
}

void AWeapon::EnableVisuals(const bool bEnable)
{
	Super::EnableVisuals(bEnable);

	
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeapon::AttachMeshToSocket(USceneComponent* InParent)
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

void AWeapon::EnableWeaponState(const bool bEnable) const
{
	if (CharacterStateProvider)
	{
		const ECharacterWeaponStates NewState = bEnable ? ECharacterWeaponStates::ECWS_EquippedWeapon : ECharacterWeaponStates::ECWS_Unequipped;
		CharacterStateProvider->SetWeaponState(NewState);
	}
}
