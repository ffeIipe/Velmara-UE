// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zone.generated.h"

class ALever;
class ATrigger;
class AEnemySpawner;

UCLASS()
class TESISUE_API AZone : public AActor
{
	GENERATED_BODY()

public:
	AZone();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties")
	AEnemySpawner* EnemySpawner = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties")
	ATrigger* Trigger = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Properties")
	TArray<ALever*> Levers;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnLeverActivation();

	bool CheckLevers();
};

