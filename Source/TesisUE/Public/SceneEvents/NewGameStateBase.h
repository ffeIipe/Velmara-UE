// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

	UFUNCTION(BlueprintCallable, Category = "Memento", BlueprintPure)
	TArray<AActor*> GetMementoEntities() const;

	UFUNCTION(BlueprintCallable, Category = "Memento")
	void SaveAllMementoStates();

	UFUNCTION(BlueprintCallable, Category = "Memento")
	void LoadAllMementoStates();

private:
	TArray<AActor*> MementoEntities;
};
