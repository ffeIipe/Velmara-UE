#include "SceneEvents/VelmaraGameStateBase.h"

#include "EngineUtils.h"
#include "Components/MementoComponent.h"
#include "Entities/Entity.h"
#include "Interfaces/MementoEntity.h"
#include "Items/Item.h"

void AVelmaraGameStateBase::RegisterMementoEntity(AEntity* Entity)
{
	if (Entity && !MementoEntities.Contains(Entity))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, Entity->GetName() + " registered.");

		MementoEntities.Add(Entity);
	}
}

void AVelmaraGameStateBase::RegisterMementoItem(AItem* Item)
{
	if (Item && !MementoItems.Contains(Item) && !Item->bWasUsed)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, Item->GetName() + " registered.");

		MementoItems.Add(Item);
	}
}

void AVelmaraGameStateBase::UnregisterMementoEntity(AEntity* Entity)
{
	if (Entity && Entity->Execute_GetMementoComponent(Entity))
	{
		MementoEntities.Remove(Entity);
	}
}

TArray<AEntity*> AVelmaraGameStateBase::GetMementoEntities()
{
	for (TActorIterator<AEntity> It(GetWorld()); It; ++It)
	{
		AEntity* Entity = *It;
		MementoEntities.Add(Entity);
	}
	
	return MementoEntities;
}

TArray<AItem*> AVelmaraGameStateBase::GetMementoItems()
{
	for (TActorIterator<AItem> It(GetWorld()); It; ++It)
	{
		AItem* Item = *It;
		MementoItems.Add(Item);
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
		Result.Add(Item->GetUniqueSaveID(), Item->GetItemMementoComponent()->GetInternalItemState());
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
	return Result;
}

void AVelmaraGameStateBase::LoadAllMementoStates()
{
	for (AActor* Entity : MementoEntities)
	{
		if (!IsValid(Entity)) return;
		
		if (const TScriptInterface<IMementoEntity> MementoEntity = Entity)
		{
			if (UMementoComponent* MementoComponent = MementoEntity->Execute_GetMementoComponent(Entity);
				IsValid(MementoComponent))
			{
				MementoComponent->ApplyExternalState(MementoComponent->GetInternalSavedState());
			}
		}
	}
}

void AVelmaraGameStateBase::InitializeItems(TArray<FItemMementoState> ItemsStates)
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Black, "Initializing Items data.");
	
	WorldItemsStates.Empty();
	for (const FItemMementoState& ItemState : ItemsStates)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, "Processing item data: " + ItemState.UniqueSaveID.ToString());

		WorldItemsStates.Add(ItemState.UniqueSaveID, ItemState);
	}

	for (const AItem* Item : GetMementoItems())
	{
		if (const FItemMementoState* ItemData = WorldItemsStates.Find(Item->GetUniqueSaveID()))
		{
			const FItemMementoState& ItemState = *ItemData;
			Item->GetItemMementoComponent()->ApplyExternalState(ItemState);
		}
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
		const FEntityMementoState& ItemData = *WorldEntityStates.Find(Entity->GetUniqueSaveID());
		Entity->Execute_GetMementoComponent(Entity)->ApplyExternalState(ItemData);
	}
}

TArray<FItemMementoState> AVelmaraGameStateBase::SaveAllItemMementoStates()
{
	TArray<FItemMementoState> Result;

	for (const AItem* Item : GetMementoItems())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, Item->GetName() + " saved.");
		
		Result.Add(Item->GetItemMementoComponent()->CaptureItemState());
	}
	return Result;
}

// void ANewGameStateBase::ClearPendingLoadData()
// {
//     PendingEnemyLoadData.Empty();
//     bIsLoadingFromSave = false;
// }
