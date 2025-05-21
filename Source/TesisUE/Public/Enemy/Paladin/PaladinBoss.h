// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Paladin.h"
#include "PaladinBoss.generated.h"

UCLASS()
class TESISUE_API APaladinBoss : public APaladin
{
	GENERATED_BODY()

protected:
	APaladinBoss();

	void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (DisplayName = "Initial Minion Pool Size Per Boss"))
	int32 InitialMinionPoolSize = 6;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<USceneComponent*> SpawnPoints;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<APaladin> MinionToSpawnClass;

	TArray<APaladin*> Minions;
	FTimerHandle InvokeTimer;

	UFUNCTION(BlueprintCallable)
	void Attack();

	UFUNCTION(BlueprintCallable)
	void TryToInvoke();

	void Invoke();

	UFUNCTION()
	void HandleMinionDeactivated(AEnemy* DeactivatedMinion);	
};
