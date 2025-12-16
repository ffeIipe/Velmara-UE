// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyTokenManager.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API UEnemyTokenManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category = "Tokens")
    bool TryReserveAttackToken();

    UFUNCTION(BlueprintCallable, Category = "Tokens")
    void ReturnAttackToken();

protected:
    UPROPERTY(EditAnywhere, Category = "Tokens", meta = (ClampMin = "0"))
    int32 MaxAttackTokens = 2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tokens")
    int32 CurrentAvailableTokens;
};
