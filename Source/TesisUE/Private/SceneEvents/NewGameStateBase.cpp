#include "SceneEvents/NewGameStateBase.h"
#include "Components/MementoComponent.h"
#include "Interfaces/MementoEntity.h"
#include "Items/Item.h"
#include "EngineUtils.h"

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

void ANewGameStateBase::InitializeWorldInteractedItemsState(const TArray<FInteractedItemSaveData>& InteractedItemData)
{
	UE_LOG(LogTemp, Warning, TEXT("GameState: InitializeWorldInteractedItemsState BEGIN. Received %d items from save file."), InteractedItemData.Num());
	WorldInteractedItemsStates.Empty(); // Limpiamos estados anteriores
	for (const FInteractedItemSaveData& Data : InteractedItemData)
	{
		UE_LOG(LogTemp, Log, TEXT("GameState: Storing item data from save - ID: %s, bWasOpened: %s"), *Data.UniqueSaveID.ToString(), Data.bWasOpened ? TEXT("true") : TEXT("false"));
		WorldInteractedItemsStates.Add(Data.UniqueSaveID, Data);
	}
	UE_LOG(LogTemp, Warning, TEXT("GameState: InitializeWorldInteractedItemsState - Finished storing %d items."), WorldInteractedItemsStates.Num());

	// NUEVA LÓGICA: Iterar sobre los ítems del mundo y pedirles que se reconcilien AHORA que tenemos los datos.
	UE_LOG(LogTemp, Warning, TEXT("GameState: Now attempting to reconcile all AItem instances in the world AFTER loading states."));
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AItem> It(World); It; ++It)
		{
			AItem* WorldItem = *It;
			if (IsValid(WorldItem))
			{
				UE_LOG(LogTemp, Log, TEXT("GameState: Re-checking item '%s' (ID: %s) post-initialization."), *WorldItem->GetName(), *WorldItem->GetUniqueSaveID().ToString());
				// Usamos una función de reconciliación un poco modificada o la misma si es segura.
				// Lo importante es que ahora WorldInteractedItemsStates ESTÁ POBLADO.
				FInteractedItemSaveData* SavedData = WorldInteractedItemsStates.Find(WorldItem->GetUniqueSaveID());
				if (SavedData)
				{
					UE_LOG(LogTemp, Warning, TEXT("GameState: Found data for item '%s' (ID: %s) during post-init reconciliation. Applying state (bWasOpened: %s)."), *WorldItem->GetName(), *WorldItem->GetUniqueSaveID().ToString(), SavedData->bWasOpened ? TEXT("true") : TEXT("false"));
					WorldItem->ApplySavedState(SavedData);
				}
				else
				{
					// Si AÚN no hay datos (por ejemplo, un ítem nuevo que no estaba en el save), lo registramos.
					// Pero esto no debería pasar para ítems que estaban en el save.
					UE_LOG(LogTemp, Log, TEXT("GameState: Still NO data for item '%s' (ID: %s) during post-init reconciliation. Registering with current state (bWasOpened: %s)."), *WorldItem->GetName(), *WorldItem->GetUniqueSaveID().ToString(), WorldItem->bWasOpened ? TEXT("true") : TEXT("false"));
					FInteractedItemSaveData InitialData;
					InitialData.UniqueSaveID = WorldItem->GetUniqueSaveID();
					InitialData.bWasOpened = WorldItem->bWasOpened; // Su estado actual en el mundo
					// No llamamos a UpdateInteractedItemState aquí para no ensuciar los datos recién cargados,
					// a menos que sea un ítem completamente nuevo no presente en el save.
					// Por ahora, nos enfocamos en que los ítems del save se destruyan.
				}
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("GameState: InitializeWorldInteractedItemsState END and post-reconciliation attempt finished."));
}


void ANewGameStateBase::UpdateInteractedItemState(const FInteractedItemSaveData& UpdatedItemData)
{
	UE_LOG(LogTemp, Warning, TEXT("GameState: UpdateInteractedItemState called for ID: %s. New bWasOpened state: %s"), *UpdatedItemData.UniqueSaveID.ToString(), UpdatedItemData.bWasOpened ? TEXT("true") : TEXT("false"));
	WorldInteractedItemsStates.FindOrAdd(UpdatedItemData.UniqueSaveID) = UpdatedItemData;
}


void ANewGameStateBase::RequestInteractedItemStateReconciliation(AItem* ItemToReconcile)
{
	if (!IsValid(ItemToReconcile)) return;

	FName ItemID = ItemToReconcile->GetUniqueSaveID();
	UE_LOG(LogTemp, Log, TEXT("GameState: RequestInteractedItemStateReconciliation for item '%s' (ID: %s)"), *ItemToReconcile->GetName(), *ItemID.ToString());

	// Esta función es llamada por AItem::BeginPlay.
	// En este punto, WorldInteractedItemsStates PUEDE AÚN NO ESTAR LLENO con los datos del savegame
	// si el BeginPlay ocurre antes de que ApplyPendingLoadedDataToWorld llame a InitializeWorldInteractedItemsState.
	FInteractedItemSaveData* SavedData = WorldInteractedItemsStates.Find(ItemID);

	if (SavedData)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameState: Found saved data for ID: %s during item's BeginPlay. Applying state (bWasOpened: %s)."), *ItemID.ToString(), SavedData->bWasOpened ? TEXT("true") : TEXT("false"));
		ItemToReconcile->ApplySavedState(SavedData);
	}
	else
	{
		// Si no se encuentran datos guardados, podría ser porque:
		// 1. Es un ítem nuevo en el mundo que no estaba en el save.
		// 2. O, este BeginPlay se ejecutó ANTES de que InitializeWorldInteractedItemsState cargara los datos.
		UE_LOG(LogTemp, Warning, TEXT("GameState: NO saved data found for ID: %s during item's BeginPlay. Registering with its current state (bWasOpened: %s). This might be overridden later if data is loaded."), *ItemID.ToString(), ItemToReconcile->bWasOpened ? TEXT("true") : TEXT("false"));
		FInteractedItemSaveData InitialData;
		InitialData.UniqueSaveID = ItemID;
		InitialData.bWasOpened = ItemToReconcile->bWasOpened;
		// Es crucial NO modificar WorldInteractedItemsStates aquí si esperamos que se cargue desde el save.
		// El registro inicial ya lo hace el propio BeginPlay si no encuentra nada.
		// Podríamos simplemente no hacer nada aquí si esperamos una carga posterior, o solo registrarlo si NO estamos en proceso de carga.
		// Por simplicidad, dejaremos que se registre, y la reconciliación post-carga en InitializeWorldInteractedItemsState lo corregirá.
		if (!WorldInteractedItemsStates.Contains(ItemID)) // Solo lo añade si no existe para evitar sobreescribir datos cargados pero aún no procesados.
		{
			UpdateInteractedItemState(InitialData); // Esta función sí actualiza el TMap.
		}
	}
}

void ANewGameStateBase::GetAllInteractedItemStates(TArray<FInteractedItemSaveData>& OutItemStates)
{
	WorldInteractedItemsStates.GenerateValueArray(OutItemStates);
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
