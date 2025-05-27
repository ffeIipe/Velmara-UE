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

	// Función para recibir los datos de carga desde el GameInstance
	void SetPendingEnemyLoadData(const TArray<FEnemySaveData>& EnemyData);

	// Función que cada enemigo llamará para obtener su estado
	void RequestEnemyStateReconciliation(AEnemy* EnemyToReconcile);

protected:
	// "Pizarrón" con los datos de enemigos pendientes de cargar
	TMap<FName, FEnemySaveData> PendingEnemyLoadData;

	// Flag para saber si estamos en proceso de carga
	bool bIsLoadingFromSave = false;

private:
	UPROPERTY()
	TArray<AActor*> MementoEntities;
};
