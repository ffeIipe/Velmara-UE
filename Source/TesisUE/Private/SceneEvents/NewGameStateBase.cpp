#include "SceneEvents/NewGameStateBase.h"
#include "Components/MementoComponent.h"
#include "Interfaces/MementoEntity.h"

void ANewGameStateBase::RegisterMementoEntity(AActor* Entity)
{
	if (Entity && !MementoEntities.Contains(Entity))
	{
		if (IMementoEntity* MementoEntity = Cast<IMementoEntity>(Entity))
		{
			UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);
			if (IsValid(MementoComponent))
			{
				MementoEntities.Add(Entity);
				//Entity->OnDestroyed.AddDynamic(this, &ANewGameStateBase::OnMementoEntityDestroyed);
			}
		}	
	}
}

void ANewGameStateBase::UnregisterMementoEntity(AActor* Entity)
{
	if (Entity)
	{
		if (IMementoEntity* MementoEntity = Cast<IMementoEntity>(Entity))
		{
			UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);
			if (IsValid(MementoComponent))
			{
				MementoEntities.Remove(Entity);
			}
		}
	}
}

TArray<AActor*> ANewGameStateBase::GetMementoEntities() const
{
	return MementoEntities;
}

void ANewGameStateBase::SaveAllMementoStates()
{
	for (AActor* Entity : MementoEntities)
	{
		if (IsValid(Entity))
		{
			if (IMementoEntity* MementoEntity = Cast<IMementoEntity>(Entity))
			{
				UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);
				if (IsValid(MementoComponent))
				{
					MementoComponent->SaveState();
				}
			}
		}
	}
}

void ANewGameStateBase::LoadAllMementoStates()
{
	for (AActor* Entity : MementoEntities)
	{
		if (IsValid(Entity))
		{
			if (IMementoEntity* MementoEntity = Cast<IMementoEntity>(Entity))
			{
				UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);

				if (IsValid(MementoComponent))
				{
					MementoComponent->LoadState();
				}
			}
		}
	}
}

void ANewGameStateBase::SetPendingEnemyLoadData(const TArray<FEnemySaveData>& EnemyData)
{
	PendingEnemyLoadData.Empty();
	for (const FEnemySaveData& Data : EnemyData)
	{
		PendingEnemyLoadData.Add(Data.EnemyID, Data);
	}
	bIsLoadingFromSave = !PendingEnemyLoadData.IsEmpty();

	// Opcional: Limpiar los datos después de un tiempo para evitar que afecten a enemigos spawneados mucho después.
	// FTimerHandle TempHandle;
	// GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &ANewGameStateBase::ClearPendingLoadData, 10.f, false);
}

void ANewGameStateBase::RequestEnemyStateReconciliation(AEnemy* EnemyToReconcile)
{
	if (!bIsLoadingFromSave || !IsValid(EnemyToReconcile))
	{
		return;
	}

	FName EnemyID = EnemyToReconcile->GetFName();
	FEnemySaveData* SavedData = PendingEnemyLoadData.Find(EnemyID);

	if (SavedData)
	{
		// El enemigo existe en los datos guardados, aplicamos su estado.
		if (SavedData->bIsAlive)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Reconciliando %s: VIVO"), *EnemyID.ToString()));

			// Re-activamos y aplicamos estado. Es importante llamar a ActivateEnemy primero para resetearlo.
			EnemyToReconcile->ActivateEnemy(SavedData->EnemyState.Transform.GetLocation(), SavedData->EnemyState.Transform.GetRotation().Rotator());

			UMementoComponent* MementoComp = EnemyToReconcile->FindComponentByClass<UMementoComponent>();
			if (MementoComp)
			{
				MementoComp->ApplyExternalState(SavedData->EnemyState);
			}
		}
		else
		{
			// Debe estar muerto.
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Reconciliando %s: MUERTO"), *EnemyID.ToString()));
			EnemyToReconcile->DeactivateEnemy();
		}
	}
	else
	{
		// El enemigo existe en el nivel pero no en el guardado. Lo desactivamos.
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Reconciliando %s: NO ENCONTRADO. Desactivando."), *EnemyID.ToString()));
		EnemyToReconcile->DeactivateEnemy();
	}
}

// void ANewGameStateBase::ClearPendingLoadData()
// {
//     PendingEnemyLoadData.Empty();
//     bIsLoadingFromSave = false;
// }
