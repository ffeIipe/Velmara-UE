#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "Features/SaveSystem/Data/SaveTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"
#include "EngineUtils.h"
#include "Features/SaveSystem/Core/SaveComponent.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/CustomPlayerController.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void USaveGameSubsystem::SerializeProperties(UObject* Object, TArray<uint8>& OutBytes)
{
    if (!IsValid(Object)) return;

    FMemoryWriter Writer(OutBytes);
    FObjectAndNameAsStringProxyArchive Ar(Writer, false);
    Ar.ArIsSaveGame = true;

    for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasAnyPropertyFlags(CPF_SaveGame) && 
        !Property->IsA<FObjectPropertyBase>() && 
        !Property->IsA<FInterfaceProperty>())
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
    
            Property->SerializeItem(FStructuredArchiveFromArchive(Ar).GetSlot(), ValuePtr);
        }
    }
}

void USaveGameSubsystem::DeserializeProperties(UObject* Object, const TArray<uint8>& InBytes)
{
    if (!IsValid(Object) || InBytes.Num() == 0) return;

    FMemoryReader Reader(InBytes);
    FObjectAndNameAsStringProxyArchive Ar(Reader, false);
    Ar.ArIsSaveGame = true;

    for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        
        if (Property->HasAnyPropertyFlags(CPF_SaveGame) && 
    !Property->IsA<FObjectPropertyBase>() && 
    !Property->IsA<FInterfaceProperty>())
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
    
            Property->SerializeItem(FStructuredArchiveFromArchive(Ar).GetSlot(), ValuePtr);
        }
    }
}

void USaveGameSubsystem::SaveGame(int32 SlotIndex)
{
    CurrentSlotIndex = SlotIndex;
    FString TargetSlotName = GetSlotNameFromIndex(SlotIndex);

    UPlayerProgressSaveGame* SaveObject = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
    
    SaveObject->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
    SaveObject->SaveSlotUserLabel = TargetSlotName;
    SaveObject->Timestamp = FDateTime::Now();

    SaveLevelActors(SaveObject);
    SavePlayerProgression(SaveObject);

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, TargetSlotName, 0);
    
    if(bSaved) 
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("SUCCESS: Partida GUARDADA en slot: %s"), *TargetSlotName));
        //OnSaveDataUpdated.Broadcast();
    }
    OnGameSaved.Broadcast(bSaved);
}

void USaveGameSubsystem::LoadGame(int32 SlotIndex)
{
    FString TargetSlotName = GetSlotNameFromIndex(SlotIndex);

    if (!UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, FString::Printf(TEXT("LOAD ABORTED: No existe %s.sav"), *TargetSlotName));
        return;
    }

    CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlotName, 0));
    
    if (!CurrentSaveGame) return;

    CurrentSlotIndex = SlotIndex;

    const FString SavedLevelName = CurrentSaveGame->CurrentLevelName;
    const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

    if (CurrentLevelName.Equals(SavedLevelName) || SavedLevelName.IsEmpty())
    {
        RestoreCurrentLevelState();
        OnGameLoaded.Broadcast(true);
    }
    else
    {
        bRestoreOnNextLevel = true;
        UGameplayStatics::OpenLevel(this, FName(*SavedLevelName));
    }
}

void USaveGameSubsystem::RestorePendingLevelState()
{
    if (!bRestoreOnNextLevel) return;

    bRestoreOnNextLevel = false;
    RestoreCurrentLevelState();
}

void USaveGameSubsystem::CreateNewGame(int32 SlotIndex, const FString MapName)
{
    if (MapName.IsEmpty())
    {
        return;
    }

    CurrentSlotIndex = SlotIndex;
    FString TargetSlotName = GetSlotNameFromIndex(SlotIndex);

    CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
	
    if (CurrentSaveGame)
    {
        CurrentSaveGame->SaveSlotUserLabel = TargetSlotName;
        CurrentSaveGame->Timestamp = FDateTime::Now();
		
        CurrentSaveGame->CurrentLevelName = MapName;

        const bool bSaved = UGameplayStatics::SaveGameToSlot(CurrentSaveGame, TargetSlotName, 0);
		
        if (bSaved)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Nueva Partida en Slot %d. Cargando: %s"), SlotIndex, *MapName));
            //OnSaveDataUpdated.Broadcast();
        }

        UGameplayStatics::OpenLevel(GetWorld(), FName(*MapName));
    }
}

bool USaveGameSubsystem::DoesSaveGameExist(int32 SlotIndex) const
{
    FString TargetSlotName = GetSlotNameFromIndex(SlotIndex);
    return UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0);
}

bool USaveGameSubsystem::GetSaveSlotInfo(int32 SlotIndex, FString& OutLevelName, FDateTime& OutTimestamp, FString& OutSlotLabel) const
{
    FString TargetSlotName = GetSlotNameFromIndex(SlotIndex);

    if (!UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0)) return false;

    USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(TargetSlotName, 0);
    if (UPlayerProgressSaveGame* ProgressSave = Cast<UPlayerProgressSaveGame>(LoadedSave))
    {
        OutLevelName = ProgressSave->CurrentLevelName;
        OutTimestamp = ProgressSave->Timestamp;
        OutSlotLabel = ProgressSave->SaveSlotUserLabel;
        return true;
    }
    return false;
}

void USaveGameSubsystem::RestoreCurrentLevelState() const
{
    if (CurrentSaveGame)
    {
        FString CurrentMap = GetWorld()->GetMapName();
        CurrentMap.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        LoadPlayerProgression(CurrentSaveGame);
        LoadLevelActors(CurrentSaveGame);
    }
}

FString USaveGameSubsystem::GetSlotNameFromIndex(int32 SlotIndex) const
{
    return FString::Printf(TEXT("SaveSlot_%d"), SlotIndex);
}

void USaveGameSubsystem::SaveLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
    int32 SavedCount = 0;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        
        if (USaveComponent* SaveComp = Actor->FindComponentByClass<USaveComponent>())
        {
            if (!SaveComp->UniqueSaveID.IsValid()) continue;

            FEntitySaveData ActorData;
            ActorData.UniqueSaveID = SaveComp->UniqueSaveID;
            ActorData.Transform = Actor->GetActorTransform();
            ActorData.ActorClass = Actor->GetClass();

            if (Actor->Implements<USaveInterface>())
            {
                FEntitySaveData InterfaceData; 
                ISaveInterface::Execute_OnSaveGame(Actor, InterfaceData);

                ActorData.ByteData = InterfaceData.ByteData;
                ActorData.SavedStats = InterfaceData.SavedStats;
            }

            SaveObject->SavedActors.Add(ActorData.UniqueSaveID, ActorData);
            SavedCount++;
        }
    }
    
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("DATA: Se guardaron %d actores válidos."), SavedCount));
}

void USaveGameSubsystem::LoadLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
    if (!SaveObject) return;

    int32 RestoredStaticCount = 0;
    int32 RestoredDynamicCount = 0;

    TArray<FGuid> ProcessedIDs;
    TArray<AActor*> RestoredActors;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;

        if (USaveComponent* SaveComp = Actor->FindComponentByClass<USaveComponent>())
        {
            FGuid ActorID = SaveComp->UniqueSaveID;

            if (SaveObject->SavedActors.Contains(ActorID))
            {
                const FEntitySaveData& Data = SaveObject->SavedActors[ActorID];
                // El transform guardado es de mundo: soltamos al actor de cualquier padre (arma, host...) antes de moverlo.
                Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                Actor->SetActorTransform(Data.Transform, false, nullptr, ETeleportType::TeleportPhysics);

                if (Actor->Implements<USaveInterface>())
                {
                    ISaveInterface::Execute_OnLoadGame(Actor, Data);
                }

                RestoredActors.Add(Actor);
                ProcessedIDs.Add(ActorID);
                RestoredStaticCount++;
            }
        }
    }

    //Dynamics objects
    for (const auto& Pair : SaveObject->SavedActors)
    {
        FGuid SavedID = Pair.Key;

        if (!ProcessedIDs.Contains(SavedID))
        {
            const FEntitySaveData& Data = Pair.Value;

            if (Data.ActorClass)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                AActor* NewActor = GetWorld()->SpawnActor<AActor>(Data.ActorClass, Data.Transform, SpawnParams);

                if (NewActor)
                {
                    if (USaveComponent* SaveComp = NewActor->FindComponentByClass<USaveComponent>())
                    {
                        SaveComp->UniqueSaveID = SavedID;
                    }

                    if (NewActor->Implements<USaveInterface>())
                    {
                        ISaveInterface::Execute_OnLoadGame(NewActor, Data);
                    }
                    RestoredActors.Add(NewActor);
                    RestoredDynamicCount++;
                }
            }
        }
    }

    for (AActor* Restored : RestoredActors)
    {
        if (Restored->Implements<USaveInterface>())
        {
            ISaveInterface::Execute_OnPostGameLoaded(Restored);
        }
    }

    if (SaveObject->PossessedEntityID.IsValid())
    {
        ACustomPlayerController* PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
        for (AActor* Restored : RestoredActors)
        {
            const USaveComponent* SaveComp = Restored->FindComponentByClass<USaveComponent>();
            if (PC && SaveComp && SaveComp->UniqueSaveID == SaveObject->PossessedEntityID)
            {
                if (APawn* Host = Cast<APawn>(Restored))
                {
                    PC->PerformPossession(Host);
                }
                break;
            }
        }
    }

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Emerald, FString::Printf(TEXT("SUCCESS: Se restauraron %d Estáticos y %d Dinámicos!"), RestoredStaticCount, RestoredDynamicCount));
}

void USaveGameSubsystem::SavePlayerProgression(UPlayerProgressSaveGame* SaveObject) const
{
    if (!SaveObject) return;

    SaveObject->PossessedEntityID.Invalidate();
    if (const ACustomPlayerController* PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        SaveObject->PlayerControlRotation = PC->GetControlRotation();

        // IsPossessing() == (pawn actual != OriginalBody); OriginalBody es protected.
        if (PC->IsPossessing() && PC->GetPawn())
        {
            if (const USaveComponent* SaveComp = PC->GetPawn()->FindComponentByClass<USaveComponent>())
            {
                SaveObject->PossessedEntityID = SaveComp->UniqueSaveID;
            }
        }
    }

    if (const UAttributeTreeComponent* AttributeTree = FindPlayerComponent<UAttributeTreeComponent>())
    {
        SaveObject->AttributeTreeLevels = AttributeTree->GetUpgradeLevels();
    }
    else if (CurrentSaveGame)
    {
        // Sin árbol en el mundo no hay niveles nuevos: conservamos los de la partida cargada.
        SaveObject->AttributeTreeLevels = CurrentSaveGame->AttributeTreeLevels;
    }

    if (const UWeaponUpgradeComponent* WeaponUpgrades = FindPlayerComponent<UWeaponUpgradeComponent>())
    {
        SaveObject->UnlockedWeaponCombos = WeaponUpgrades->GetUnlockedCombos();
    }
    else if (CurrentSaveGame)
    {
        SaveObject->UnlockedWeaponCombos = CurrentSaveGame->UnlockedWeaponCombos;
    }
}

void USaveGameSubsystem::LoadPlayerProgression(const UPlayerProgressSaveGame* SaveObject) const
{
    if (!SaveObject) return;

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        PC->SetControlRotation(SaveObject->PlayerControlRotation);
    }

    if (UAttributeTreeComponent* AttributeTree = FindPlayerComponent<UAttributeTreeComponent>())
    {
        AttributeTree->RestoreUpgradeLevels(SaveObject->AttributeTreeLevels);
    }

    if (UWeaponUpgradeComponent* WeaponUpgrades = FindPlayerComponent<UWeaponUpgradeComponent>())
    {
        WeaponUpgrades->RestoreUnlockedCombos(SaveObject->UnlockedWeaponCombos);
    }
}

template <typename TComponent>
TComponent* USaveGameSubsystem::FindPlayerComponent() const
{
    if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        if (TComponent* Component = PlayerPawn->FindComponentByClass<TComponent>())
        {
            return Component;
        }
    }

    // El jugador puede estar poseyendo otra entidad: buscamos el componente en el personaje jugable.
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        if (TComponent* Component = It->FindComponentByClass<TComponent>())
        {
            return Component;
        }
    }

    return nullptr;
}