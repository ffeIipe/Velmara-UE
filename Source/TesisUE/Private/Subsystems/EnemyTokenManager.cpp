// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/EnemyTokenManager.h"

void UEnemyTokenManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentAvailableTokens = MaxAttackTokens;
}

void UEnemyTokenManager::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    CurrentAvailableTokens = MaxAttackTokens;
}

bool UEnemyTokenManager::TryReserveAttackToken()
{
    if (CurrentAvailableTokens > 0)
    {
        CurrentAvailableTokens--;
        return true;
    }
    return false;
}

void UEnemyTokenManager::ReturnAttackToken()
{
    CurrentAvailableTokens++;
    CurrentAvailableTokens = FMath::Min(CurrentAvailableTokens, MaxAttackTokens);
}