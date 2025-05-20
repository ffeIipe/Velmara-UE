// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Paladin.h"
#include "PaladinBoss.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API APaladinBoss : public APaladin
{
	GENERATED_BODY()
protected:
	APaladinBoss();

	void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<USceneComponent*> SpawnPoints;

private:
	TArray<APaladin*> Minions;

	UFUNCTION(BlueprintCallable)
	void Attack();

	void Invoke();

	void RemoveMinion(AActor* ActorToRemove);
};
