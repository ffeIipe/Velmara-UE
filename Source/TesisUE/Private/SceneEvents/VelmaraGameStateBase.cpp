#include "SceneEvents/VelmaraGameStateBase.h"

#include "EngineUtils.h"
#include "Components/MementoComponent.h"
#include "Entities/Entity.h"
#include "Interfaces/MementoEntity.h"
#include "Items/Item.h"

void AVelmaraGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	GetMementoEntities();
	GetMementoItems();
}

void AVelmaraGameStateBase::RegisterMementoEntity(AEntity* Entity)
{
	if (Entity && !MementoEntities.Contains(Entity))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, Entity->GetName() + " registered.");

		MementoEntities.AddUnique(Entity);
	}
}

void AVelmaraGameStateBase::RegisterMementoItem(AItem* Item)
{
	if (Item && !MementoItems.Contains(Item) && !Item->bWasUsed)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, Item->GetName() + " registered.");

		MementoItems.AddUnique(Item);
	}
}

void AVelmaraGameStateBase::UnregisterMementoEntity(AEntity* Entity)
{
	if (Entity && Entity->Execute_GetMementoComponent(Entity))
	{
		MementoEntities.Remove(Entity);
	}
}

void AVelmaraGameStateBase::UpdateEntityState(AEntity* DeadEntity)
{
	const FEntityMementoState DeadEntityState = DeadEntity->Execute_GetMementoComponent(DeadEntity)->CaptureOwnerState();
	if (WorldEntityStates.Find(DeadEntity->GetUniqueSaveID()))
	{
		WorldEntityStates[DeadEntity->GetUniqueSaveID()] = DeadEntityState;
	}

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, DeadEntity->GetName() + " updated.  [" + DeadEntity->GetUniqueSaveID().ToString() + "]");
}

TArray<AEntity*> AVelmaraGameStateBase::GetMementoEntities()
{
	//MementoEntities.Empty();

	for (TActorIterator<AEntity> It(GetWorld()); It; ++It)
	{
		AEntity* Entity = *It;
		MementoEntities.AddUnique(Entity);

		Entity->OnDead.AddDynamic(this, &AVelmaraGameStateBase::UpdateEntityState);

		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, Entity->GetName() + " registered.");
	}
	return MementoEntities;
}

TArray<AItem*> AVelmaraGameStateBase::GetMementoItems()
{
	//MementoItems.Empty();

	for (TActorIterator<AItem> It(GetWorld()); It; ++It)
	{
		AItem* Item = *It;
		MementoItems.AddUnique(Item);
	}
	return MementoItems;
}

TMap<FName, FEntityMementoState> AVelmaraGameStateBase::GetEntityMementoStatesWithKey()
{
	TMap<FName, FEntityMementoState> Result;

	for (AEntity* Entity : GetMementoEntities())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Cyan, "Entity ID: " + Entity->GetUniqueSaveID().ToString() + " - " + Entity->GetName());

		Result.Add(Entity->GetUniqueSaveID(), Entity->Execute_GetMementoComponent(Entity)->GetInternalSavedState());
	}
	return Result;
}

TMap<FName, FItemMementoState> AVelmaraGameStateBase::GetItemMementoStatesWithKey()
{
	TMap<FName, FItemMementoState> Result;

	for (const AItem* Item : GetMementoItems())
	{
		Result.Add(Item->GetUniqueSaveID(), Item->GetItemMementoComponent()->CaptureItemState());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, Item->GetUniqueSaveID().ToString());
	}
	return Result;
}

TArray<FEntityMementoState> AVelmaraGameStateBase::SaveAllEntityMementoStates()
{
	TArray<FEntityMementoState> Result;

	for (AActor* Entity : GetMementoEntities())
	{
		if (const TScriptInterface<IMementoEntity> MementoEntity = Entity)
		{
			if (UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);
				IsValid(MementoComponent))
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, Entity->GetName() + " saved.");

				Result.Add(MementoComponent->CaptureOwnerState());
			}
		}
	}
	
	CurrentGameEntities = Result;
	return Result;
}

void AVelmaraGameStateBase::InitializeItems(const TArray<FItemMementoState>& ItemsStates)
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Black, "Initializing Items data.");
	
	WorldItemsStates.Empty();
	for (const FItemMementoState& ItemState : ItemsStates)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, "Processing item data: " + ItemState.UniqueSaveID.ToString());

		WorldItemsStates.Add(ItemState.UniqueSaveID, ItemState);
	}

	for (AItem* Item : GetMementoItems())
	{
		if (const FItemMementoState* ItemData = WorldItemsStates.Find(Item->GetUniqueSaveID()))
		{
			/*if (ItemData->bWasOpened)
			{
				Item->Destroy();
			}*/

			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Turquoise, Item->GetUniqueSaveID().ToString());
			
			const FItemMementoState& ItemState = *ItemData;
			Item->GetItemMementoComponent()->ApplyExternalState(ItemState);
		}
		else GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, Item->GetUniqueSaveID().ToString());
	}
}

void AVelmaraGameStateBase::InitializeEntities(TArray<FEntityMementoState> EntitiesStates)
{
	WorldEntityStates.Empty();
	for (const FEntityMementoState& EntityData : EntitiesStates)
	{
		WorldEntityStates.Add(EntityData.UniqueSaveID, EntityData);
	}

	for (AEntity* Entity : GetMementoEntities())
	{
		const FEntityMementoState& EntityData = *WorldEntityStates.Find(Entity->GetUniqueSaveID());
		Entity->Execute_GetMementoComponent(Entity)->ApplyExternalState(EntityData);
	}
}

TArray<FItemMementoState> AVelmaraGameStateBase::SaveAllItemMementoStates()
{
	TArray<FItemMementoState> Result;

	for (const AItem* Item : GetMementoItems())
	{
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, Item->GetName() + " saved.");
			*/

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, Item->GetUniqueSaveID().ToString());
		Result.Add(Item->GetItemMementoComponent()->CaptureItemState());
	}
	
	CurrentGameItems = Result;
	return Result;
}

// void ANewGameStateBase::ClearPendingLoadData()
// {
//     PendingEnemyLoadData.Empty();
//     bIsLoadingFromSave = false;
// }
