// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <LoadSystem/PlayerProgressSaveGame.h>
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NewGameStateBase.generated.h"

UCLASS()
class TESISUE_API ANewGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Memento")
	void RegisterMementoEntity(AActor* Entity);
	
	UFUNCTION(BlueprintCallable, Category = "Memento")
	void UnregisterMementoEntity(AActor* Entity);

	UFUNCTION(BlueprintCallable, Category = "Memento", BlueprintPure)
	TArray<AActor*> GetMementoEntities() const;

	UFUNCTION(BlueprintCallable, Category = "Memento")
	void SaveAllMementoStates();

	UFUNCTION(BlueprintCallable, Category = "Memento")
	void LoadAllMementoStates();

	void SetPendingEnemyLoadData(const TArray<FEnemySaveData>& EnemyData);

	void RequestEnemyStateReconciliation(AEnemy* EnemyToReconcile);

	void InitializeWorldState(const TArray<FEnemySaveData>& EnemyData);

	void UpdateEnemyState(const FEnemySaveData& UpdatedEnemyData);

	void GetAllEnemyStates(TArray<FEnemySaveData>& OutEnemyStates);

protected:
	TMap<FName, FEnemySaveData> PendingEnemyLoadData;

	UPROPERTY()
	TMap<FName, FEnemySaveData> WorldEnemyStates;

	bool bIsLoadingFromSave = false;

private:
	UPROPERTY()
	TArray<AActor*> MementoEntities;
};
