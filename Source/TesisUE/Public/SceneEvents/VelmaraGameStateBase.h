// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <LoadSystem/PlayerProgressSaveGame.h>
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VelmaraGameStateBase.generated.h"

class AItem;

UCLASS()
class TESISUE_API AVelmaraGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	void RegisterMementoEntity(AEntity* Entity);
	void RegisterMementoItem(AItem* Item);
	
	void UnregisterMementoEntity(AEntity* Entity);
	
	TArray<AEntity*> GetMementoEntities();
	TArray<AItem*> GetMementoItems();

	TMap<FName, FEntityMementoState> GetEntityMementoStatesWithKey();
	TMap<FName, FItemMementoState> GetItemMementoStatesWithKey();

	UFUNCTION(BlueprintCallable, Category = "Memento")
	void LoadAllMementoStates();

	void InitializeItems(TArray<FItemMementoState> ItemsStates);
	TArray<FEntityMementoState> SaveAllEntityMementoStates();
	/*void RequestItemStateReconciliation(const AItem* Item);*/
	
	void InitializeEntities(TArray<FEntityMementoState> EntitiesStates);
	TArray<FItemMementoState> SaveAllItemMementoStates();
	/*void RequestEntityStateReconciliation(AEntity* Entity);*/

protected:
	UPROPERTY()
	TMap<FName, FEntityMementoState> WorldEntityStates;
	
	UPROPERTY()
	TMap<FName, FItemMementoState> WorldItemsStates;

	bool bIsLoadingFromSave = false;

private:
	UPROPERTY()
	TArray<AEntity*> MementoEntities;

	UPROPERTY()
	TArray<AItem*> MementoItems;
};
