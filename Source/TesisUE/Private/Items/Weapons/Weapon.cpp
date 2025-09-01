// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"

#include "LoadSystem/PlayerProgressSaveGame.h"
#include "SceneEvents/NewGameStateBase.h"
#include "Tutorial/PromptWidgetComponent.h"

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	DisableCollision();
	
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
}

void AWeapon::EnableVisuals(bool bEnable)
{
	Super::EnableVisuals(bEnable);

	
}
