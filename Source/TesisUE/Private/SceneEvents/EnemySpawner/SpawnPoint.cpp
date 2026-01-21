// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/EnemySpawner/SpawnPoint.h"

#include "Enemy/Enemy.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/EnemyPoolManager.h"


// Sets default values
ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASpawnPoint::Spawn() const
{
	/*if (UEnemyPoolManager* EnemyPoolManager = GetWorld()->GetSubsystem<UEnemyPoolManager>())
	{
		const bool bCanLoadClass = !EnemyClass.IsNull();
		const bool bClassLoaded = EnemyClass.IsValid();
		
		if (bCanLoadClass && !bClassLoaded)
		{
			 EnemyClass.LoadSynchronous();
		}
		
		if (bClassLoaded)
		{
			AEnemy* TempEnemy = EnemyPoolManager->SpawnEnemyFromPool(EnemyClass.Get(), GetActorLocation(), GetActorRotation());
			TempEnemy->OnDeactivated.AddDynamic(this, &ASpawnPoint::DecreaseEnemiesSpawned);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, "Fail to Load Class of Enemy from Spawn Point");
		}
	}*/
	
	//if (UEnemyPoolManager* EnemyPoolManager = GetWorld()->GetSubsystem<UEnemyPoolManager>())
    {
        if (EntityClass)
        {

        	FActorSpawnParameters SpawnParams;
        	SpawnParams.Owner = Owner;
        	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        	
        	GetWorld()->SpawnActor<AEntity>(EntityClass, GetActorLocation(), GetActorRotation(), SpawnParams);
        	
            /*AEnemy* TempEnemy = EnemyPoolManager->SpawnEnemyFromPool(EntityClass.Get(), GetActorLocation(), GetActorRotation());
            if (TempEnemy)
            {
                TempEnemy->OnDeactivated.AddDynamic(this, &ASpawnPoint::DecreaseEnemiesSpawned);
            }*/
        }
        /*else if (!EntityClass.IsNull())
        {
            UAssetManager& AssetManager = UAssetManager::Get();
            FStreamableManager& StreamableManager = AssetManager.GetStreamableManager();

            StreamableManager.RequestAsyncLoad(EntityClass.ToSoftObjectPath(), [this, EnemyPoolManager]()
            {
            	if (EntityClass.IsValid())
            	{
		            if (AEnemy* TempEnemy = EnemyPoolManager->SpawnEnemyFromPool(EntityClass.Get(), GetActorLocation(), GetActorRotation()))
					{
						TempEnemy->OnDeactivated.AddDynamic(this, &ASpawnPoint::DecreaseEnemiesSpawned);
					}
				}
				else
				{
					//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, TEXT("Failed to async load EnemyClass after request."));
				}
            });
        }
        else
        {
            //if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3, FColor::Red, TEXT("EnemyClass is not set on Spawn Point."));
        }*/
    }
}

void ASpawnPoint::DecreaseEnemiesSpawned(AEnemy* DeactivatedEnemy)
{
	OnEnemyDeactivated.ExecuteIfBound();
}
