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
		PendingEnemyLoadData.Add(Data.UniqueSaveID, Data);
	}
	bIsLoadingFromSave = !PendingEnemyLoadData.IsEmpty();

	// Opcional: Limpiar los datos después de un tiempo para evitar que afecten a enemigos spawneados mucho después.
	// FTimerHandle TempHandle;
	// GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &ANewGameStateBase::ClearPendingLoadData, 10.f, false);
}



void ANewGameStateBase::InitializeWorldState(const TArray<FEnemySaveData>& EnemyData)
{
	WorldEnemyStates.Empty();
	for (const FEnemySaveData& Data : EnemyData)
	{
		WorldEnemyStates.Add(Data.UniqueSaveID, Data);
	}
}

void ANewGameStateBase::UpdateEnemyState(const FEnemySaveData& UpdatedEnemyData)
{
	WorldEnemyStates.FindOrAdd(UpdatedEnemyData.UniqueSaveID) = UpdatedEnemyData;
}

void ANewGameStateBase::RequestEnemyStateReconciliation(AEnemy* EnemyToReconcile)
{
	if (!IsValid(EnemyToReconcile)) return;

	FName EnemyID = EnemyToReconcile->GetUniqueSaveID();
	FEnemySaveData* SavedData = WorldEnemyStates.Find(EnemyID);

	if (SavedData)
	{
		if (SavedData->bIsAlive)
		{
			EnemyToReconcile->ActivateEnemy(SavedData->EnemyState.Transform.GetLocation(), SavedData->EnemyState.Transform.GetRotation().Rotator());
			UMementoComponent* MementoComp = EnemyToReconcile->FindComponentByClass<UMementoComponent>();
			if (MementoComp)
			{
				MementoComp->ApplyExternalState(SavedData->EnemyState);
			}
		}
		else
		{
			EnemyToReconcile->DeactivateEnemy();
		}
	}
	else
	{
		FEnemySaveData InitialData;
		InitialData.UniqueSaveID = EnemyID;
		InitialData.bIsAlive = true;
		InitialData.EnemyClass = EnemyToReconcile->GetClass();
		if (UMementoComponent* Memento = EnemyToReconcile->FindComponentByClass<UMementoComponent>())
		{
			InitialData.EnemyState = Memento->CaptureOwnerState();
		}
		UpdateEnemyState(InitialData);
	}
}

void ANewGameStateBase::GetAllEnemyStates(TArray<FEnemySaveData>& OutEnemyStates)
{
	WorldEnemyStates.GenerateValueArray(OutEnemyStates);
}

// void ANewGameStateBase::ClearPendingLoadData()
// {
//     PendingEnemyLoadData.Empty();
//     bIsLoadingFromSave = false;
// }
