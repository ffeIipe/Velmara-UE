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
    
    SaveObject->CurrentLevelName = GetWorld()->GetMapName();
    SaveObject->SaveSlotUserLabel = SlotName;
    SaveObject->Timestamp = FDateTime::Now();

    SaveLevelActors(SaveObject);

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
    
    if(bSaved) UE_LOG(LogTemp, Log, TEXT("Game saved in slot: %s"), *SlotName);
    OnGameSaved.Broadcast(bSaved);
}

void USaveGameSubsystem::LoadGame(const FString SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return;

    CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    
    if (CurrentSaveGame)
    {
        const FString LevelName = CurrentSaveGame->CurrentLevelName;
        FString CurrentMap = GetWorld()->GetMapName();
        CurrentMap.RemoveFromStart(GetWorld()->StreamingLevelsPrefix); 

        if (CurrentMap.Contains(LevelName) || LevelName.IsEmpty())
        {
             LoadLevelActors(CurrentSaveGame);
        }
        else
        {
            UGameplayStatics::OpenLevel(this, FName(*LevelName));
        }
    }
    
    OnGameLoaded.Broadcast(true);
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
            
            ActorData.UniqueSaveID = ISaveInterface::Execute_GetUniqueSaveID(Actor);
            
            ActorData.Transform = Actor->GetActorTransform();

            ISaveInterface::Execute_OnSaveGame(Actor, ActorData);

            SaveObject->SavedActors.Add(ActorData.UniqueSaveID, ActorData);
        }
    }
}

void USaveGameSubsystem::LoadLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USaveInterface::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (FName ActorID = ISaveInterface::Execute_GetUniqueSaveID(Actor); SaveObject->SavedActors.Contains(ActorID))
        {
            const FEntitySaveData& Data = SaveObject->SavedActors[ActorID];
            
            Actor->SetActorTransform(Data.Transform);

            ISaveInterface::Execute_OnLoadGame(Actor, Data);
        }
    }
}