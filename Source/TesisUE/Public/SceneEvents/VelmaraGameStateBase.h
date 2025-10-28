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
	virtual void BeginPlay() override;
	
	void RegisterMementoEntity(AEntity* Entity);
	void RegisterMementoItem(AItem* Item);
	
	void UnregisterMementoEntity(AEntity* Entity);

	TArray<AEntity*> GetMementoEntities();
	TArray<AItem*> GetMementoItems();

	TMap<FName, FEntityMementoState> GetEntityMementoStatesWithKey();
	TMap<FName, FItemMementoState> GetItemMementoStatesWithKey();

	void InitializeItems(const TArray<FItemMementoState>& ItemsStates);
	TArray<FItemMementoState> SaveAllItemMementoStates();
	
	void InitializeEntities(TArray<FEntityMementoState> EntitiesStates);
	TArray<FEntityMementoState> SaveAllEntityMementoStates();

	UFUNCTION()
	void UpdateEntityState(AEntity* DeadEntity);

protected:
	UPROPERTY(VisibleAnywhere)
	TArray<FEntityMementoState> CurrentGameEntities;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FItemMementoState> CurrentGameItems;
	
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FEntityMementoState> WorldEntityStates;
	
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FItemMementoState> WorldItemsStates;

	bool bIsLoadingFromSave = false;

private:
	UPROPERTY()
	TArray<AEntity*> MementoEntities;

	UPROPERTY()
	TArray<AItem*> MementoItems;
};
