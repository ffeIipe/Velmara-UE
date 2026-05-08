#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "Features/SaveSystem/Data/SaveTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"
#include "GameFramework/Actor.h"

void USaveGameSubsystem::SaveGame(FString SlotName)
{
    if (SlotName.IsEmpty()) SlotName = "Slot_01";
    CurrentSlotName = SlotName;

    UPlayerProgressSaveGame* SaveObject = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
    
    SaveObject->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    SaveObject->SaveSlotUserLabel = SlotName;
    SaveObject->Timestamp = FDateTime::Now();

    SaveLevelActors(SaveObject);

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
    
    if(bSaved) 
    {
        UE_LOG(LogTemp, Log, TEXT("Game saved in slot: %s"), *SlotName);
    }
    OnGameSaved.Broadcast(bSaved);
}

void USaveGameSubsystem::LoadGame(const FString SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return;

    CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    
    if (!CurrentSaveGame) return;

    const FString SavedLevelName = CurrentSaveGame->CurrentLevelName;
    const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

    if (CurrentLevelName.Equals(SavedLevelName) || SavedLevelName.IsEmpty())
    {
        RestoreCurrentLevelState();
        OnGameLoaded.Broadcast(true);
    }
    else
    {
        UGameplayStatics::OpenLevel(this, FName(*SavedLevelName));
    }
}

void USaveGameSubsystem::RestoreCurrentLevelState() const
{
    if (CurrentSaveGame)
    {
        FString CurrentMap = GetWorld()->GetMapName();
        CurrentMap.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        LoadLevelActors(CurrentSaveGame);
    }
}

void USaveGameSubsystem::SaveLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USaveInterface::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (Actor->Implements<USaveInterface>())
        {
            FEntitySaveData ActorData;
            
            // Le preguntamos al actor su ID. Él sabrá si darnos el restaurado o el nativo.
            ActorData.UniqueSaveID = ISaveInterface::Execute_GetUniqueSaveID(Actor);
            
            ActorData.Transform = Actor->GetActorTransform();
            ActorData.ActorClass = Actor->GetClass();

            ISaveInterface::Execute_OnSaveGame(Actor, ActorData);

            SaveObject->SavedActors.Add(ActorData.UniqueSaveID, ActorData);
        }
    }
}

void USaveGameSubsystem::LoadLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
    if (!SaveObject)
    {
        UE_LOG(LogTemp, Error, TEXT("LoadLevelActors: SaveObject es nulo."));
        return;
    }

    TArray<FName> ProcessedIDs;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USaveInterface::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        FName ActorID = ISaveInterface::Execute_GetUniqueSaveID(Actor);

        if (SaveObject->SavedActors.Contains(ActorID))
        {
            const FEntitySaveData& Data = SaveObject->SavedActors[ActorID];
            
            Actor->SetActorTransform(Data.Transform, false, nullptr, ETeleportType::TeleportPhysics);
            ISaveInterface::Execute_OnLoadGame(Actor, Data);

            ProcessedIDs.Add(ActorID); 
        }
    }

    for (const auto& Pair : SaveObject->SavedActors)
    {
        FName SavedID = Pair.Key;

        if (!ProcessedIDs.Contains(SavedID))
        {
            const FEntitySaveData& Data = Pair.Value;

            if (Data.ActorClass)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                AActor* NewActor = GetWorld()->SpawnActor<AActor>(Data.ActorClass, Data.Transform, SpawnParams);

                if (NewActor && NewActor->Implements<USaveInterface>())
                {
                    ISaveInterface::Execute_OnLoadGame(NewActor, Data);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("LoadLevelActors: Intento de spawnear ID %s pero ActorClass es NULO."), *SavedID.ToString());
            }
        }
    }
}