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
#include "SceneEvents/VelmaraGameInstance.h"
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
	CurrentSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
	const FString TargetSlotName = GetSlotNameFromIndex(CurrentSlotIndex);

	UPlayerProgressSaveGame* SaveObject = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
	if (!SaveObject)
	{
		OnGameSaved.Broadcast(false);
		return;
	}

	SaveObject->SaveSlotIndex = CurrentSlotIndex;
	SaveObject->CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	SaveObject->SaveSlotUserLabel = TargetSlotName;
	SaveObject->Timestamp = FDateTime::Now();

	SaveLevelActors(SaveObject);
	SavePlayerProgression(SaveObject);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, TargetSlotName, 0);
	if (bSaved)
	{
		CurrentSaveGame = SaveObject;
		if (UVelmaraGameInstance* GameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
		{
			GameInstance->ActiveSaveSlotIndex = CurrentSlotIndex;
			GameInstance->OnSaveDataUpdated.Broadcast();
		}
	}
	OnGameSaved.Broadcast(bSaved);
}

void USaveGameSubsystem::LoadGame(int32 SlotIndex)
{
	const int32 ClampedSlot = FMath::Clamp(SlotIndex, 0, 2);
	const FString TargetSlotName = GetSlotNameFromIndex(ClampedSlot);

	if (!UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0))
	{
		OnGameLoaded.Broadcast(false);
		return;
	}

	CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlotName, 0));
	if (!CurrentSaveGame)
	{
		OnGameLoaded.Broadcast(false);
		return;
	}

	CurrentSlotIndex = ClampedSlot;
	if (UVelmaraGameInstance* GameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GameInstance->ActiveSaveSlotIndex = CurrentSlotIndex;
	}

	const FString LevelToOpen = CurrentSaveGame->CurrentLevelName.IsEmpty()
		? UGameplayStatics::GetCurrentLevelName(GetWorld(), true)
		: CurrentSaveGame->CurrentLevelName;

	if (UVelmaraGameInstance* GameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GameInstance->ShowLoadingScreen();
	}
	bRestoreOnNextLevel = true;
	UGameplayStatics::OpenLevel(this, FName(*LevelToOpen));
}

void USaveGameSubsystem::RestorePendingLevelState()
{
	if (!bRestoreOnNextLevel) return;

	bRestoreOnNextLevel = false;
	RestoreCurrentLevelState();
	OnGameLoaded.Broadcast(true);
}

void USaveGameSubsystem::CreateNewGame(int32 SlotIndex, const FString MapName)
{
	if (MapName.IsEmpty())
	{
		return;
	}

	CurrentSlotIndex = FMath::Clamp(SlotIndex, 0, 2);
	const FString TargetSlotName = GetSlotNameFromIndex(CurrentSlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(TargetSlotName, 0);
	}

	CurrentSaveGame = Cast<UPlayerProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveGame::StaticClass()));
	if (CurrentSaveGame)
	{
		CurrentSaveGame->SaveSlotIndex = CurrentSlotIndex;
		CurrentSaveGame->SaveSlotUserLabel = TargetSlotName;
		CurrentSaveGame->Timestamp = FDateTime::Now();
		CurrentSaveGame->CurrentLevelName = MapName;

		UGameplayStatics::SaveGameToSlot(CurrentSaveGame, TargetSlotName, 0);

		if (UVelmaraGameInstance* GameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
		{
			GameInstance->ActiveSaveSlotIndex = CurrentSlotIndex;
			GameInstance->OnSaveDataUpdated.Broadcast();
			GameInstance->ShowLoadingScreen();
		}

		bRestoreOnNextLevel = false;
		UGameplayStatics::OpenLevel(GetWorld(), FName(*MapName));
	}
}

bool USaveGameSubsystem::DeleteSaveSlot(int32 SlotIndex)
{
	const int32 ClampedSlot = FMath::Clamp(SlotIndex, 0, 2);
	const FString TargetSlotName = GetSlotNameFromIndex(ClampedSlot);

	bool bDeleted = false;
	if (UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0))
	{
		bDeleted = UGameplayStatics::DeleteGameInSlot(TargetSlotName, 0);
	}

	const FString LegacySlotName = FString::Printf(TEXT("PlayerProgressSlot_%d"), ClampedSlot);
	if (UGameplayStatics::DoesSaveGameExist(LegacySlotName, 0))
	{
		bDeleted |= UGameplayStatics::DeleteGameInSlot(LegacySlotName, 0);
	}

	if (CurrentSlotIndex == ClampedSlot)
	{
		CurrentSlotIndex = -1;
		CurrentSaveGame = nullptr;
	}

	if (UVelmaraGameInstance* GameInstance = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		if (GameInstance->ActiveSaveSlotIndex == ClampedSlot)
		{
			GameInstance->ActiveSaveSlotIndex = INDEX_NONE;
		}
		GameInstance->OnSaveDataUpdated.Broadcast();
	}

	OnGameSaved.Broadcast(bDeleted);
	return bDeleted;
}

bool USaveGameSubsystem::DoesSaveGameExist(int32 SlotIndex) const
{
	const FString TargetSlotName = GetSlotNameFromIndex(FMath::Clamp(SlotIndex, 0, 2));
	return UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0);
}

int32 USaveGameSubsystem::GetMostRecentSaveSlot() const
{
	int32 BestSlot = INDEX_NONE;
	FDateTime LatestTimestamp = FDateTime::MinValue();

	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		FString LevelName;
		FDateTime Timestamp;
		FString SlotLabel;
		if (GetSaveSlotInfo(SlotIndex, LevelName, Timestamp, SlotLabel))
		{
			if (BestSlot == INDEX_NONE || Timestamp > LatestTimestamp)
			{
				BestSlot = SlotIndex;
				LatestTimestamp = Timestamp;
			}
		}
	}

	return BestSlot;
}

UPlayerProgressSaveGame* USaveGameSubsystem::LoadSaveSlotObject(int32 SlotIndex) const
{
	const FString TargetSlotName = GetSlotNameFromIndex(FMath::Clamp(SlotIndex, 0, 2));
	if (!UGameplayStatics::DoesSaveGameExist(TargetSlotName, 0))
	{
		return nullptr;
	}
	return Cast<UPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(TargetSlotName, 0));
}

bool USaveGameSubsystem::GetSaveSlotInfo(int32 SlotIndex, FString& OutLevelName, FDateTime& OutTimestamp, FString& OutSlotLabel) const
{
	if (UPlayerProgressSaveGame* ProgressSave = LoadSaveSlotObject(SlotIndex))
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
		}
	}
}

void USaveGameSubsystem::LoadLevelActors(UPlayerProgressSaveGame* SaveObject) const
{
	if (!SaveObject) return;

	TArray<FGuid> ProcessedIDs;
	TArray<AActor*> RestoredActors;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (USaveComponent* SaveComp = Actor->FindComponentByClass<USaveComponent>())
		{
			const FGuid ActorID = SaveComp->UniqueSaveID;
			if (SaveObject->SavedActors.Contains(ActorID))
			{
				const FEntitySaveData& Data = SaveObject->SavedActors[ActorID];
				Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				Actor->SetActorTransform(Data.Transform, false, nullptr, ETeleportType::TeleportPhysics);

				if (Actor->Implements<USaveInterface>())
				{
					ISaveInterface::Execute_OnLoadGame(Actor, Data);
				}

				RestoredActors.Add(Actor);
				ProcessedIDs.Add(ActorID);
			}
		}
	}

	for (const auto& Pair : SaveObject->SavedActors)
	{
		const FGuid SavedID = Pair.Key;
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
}

void USaveGameSubsystem::SavePlayerProgression(UPlayerProgressSaveGame* SaveObject) const
{
	if (!SaveObject) return;

	SaveObject->PossessedEntityID.Invalidate();
	if (const ACustomPlayerController* PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		SaveObject->PlayerControlRotation = PC->GetControlRotation();
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

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (TComponent* Component = It->FindComponentByClass<TComponent>())
		{
			return Component;
		}
	}

	return nullptr;
}
