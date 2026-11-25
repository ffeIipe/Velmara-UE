#include "Subsystems/SpectralObjectsSubsystem.h"
#include "SpectralMode/Interfaces/Spectral.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace Spectral
{
	constexpr float UpdateInterval = 0.02f;
	constexpr float WarningTime = 2.0f;
	constexpr float DissolveDuration = 0.85f;
	constexpr float VanishSoundCooldown = 0.15f;
	constexpr float VanishSoundMaxDist = 2400.f;
}


void USpectralObjectsSubsystem::AddSpectralObject(AActor* Actor)
{
	if (!Actor) return;

	if (Actor->GetClass()->ImplementsInterface(USpectral::StaticClass()))
	{
		const TScriptInterface<ISpectral> SpectralInterface = Actor;
		SpectralObjects.AddUnique(SpectralInterface);

		Actor->ForEachComponent<UPrimitiveComponent>(false, [](UPrimitiveComponent* Primitive)
		{
			Primitive->SetRenderCustomDepth(true);
			Primitive->SetCustomDepthStencilValue(4);
		});
	}
}

void USpectralObjectsSubsystem::EnsureUpdateTimer()
{
	UWorld* World = GetWorld();
	if (World && !RevealedEntries.IsEmpty() && !World->GetTimerManager().IsTimerActive(UpdateTimer))
	{
		World->GetTimerManager().SetTimer(UpdateTimer, this, &USpectralObjectsSubsystem::UpdateRevealedObjects, Spectral::UpdateInterval, true);
	}
}

void USpectralObjectsSubsystem::EnsureVisualAssetsLoaded()
{
	if (!DecayOverlayMaterial)
	{
		DecayOverlayMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/BloodsenseV2/M_SpectralDecay_Overlay.M_SpectralDecay_Overlay"));
	}
	if (!DissolveMaterial)
	{
		DissolveMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/BloodsenseV2/M_SpectralDissolve.M_SpectralDissolve"));
	}
	if (!VanishNiagaraSystem)
	{
		VanishNiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/BloodsenseV2/NS_SpectralVanish.NS_SpectralVanish"));
	}
	if (!VanishSound)
	{
		VanishSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MetaSounds/MSS_Whoosh2.MSS_Whoosh2"));
	}
	if (!VanishAttenuation)
	{
		VanishAttenuation = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Audio/SoundAttenuators/SA_3D.SA_3D"));
	}
}

void USpectralObjectsSubsystem::CaptureMeshBackups(AActor* Actor, FRevealedEntry& Entry) const
{
	if (!Actor || !Entry.MeshBackups.IsEmpty()) return;

	Actor->ForEachComponent<UMeshComponent>(true, [&Entry](UMeshComponent* Mesh)
	{
		FMeshVisualBackup& Backup = Entry.MeshBackups.AddDefaulted_GetRef();
		Backup.Mesh = Mesh;
		Backup.PreviousOverlay = Mesh->GetOverlayMaterial();
		const int32 NumMaterials = Mesh->GetNumMaterials();
		Backup.OriginalMaterials.Reserve(NumMaterials);
		for (int32 Idx = 0; Idx < NumMaterials; ++Idx)
		{
			Backup.OriginalMaterials.Add(Mesh->GetMaterial(Idx));
		}
	});
}

void USpectralObjectsSubsystem::RestoreActorVisuals(AActor* Actor, FRevealedEntry& Entry)
{
	for (const FMeshVisualBackup& Backup : Entry.MeshBackups)
	{
		UMeshComponent* Mesh = Backup.Mesh.Get();
		if (!IsValid(Mesh)) continue;

		Mesh->SetOverlayMaterial(Backup.PreviousOverlay.Get());
		for (int32 Idx = 0; Idx < Backup.OriginalMaterials.Num(); ++Idx)
		{
			if (Backup.OriginalMaterials[Idx].IsValid())
			{
				Mesh->SetMaterial(Idx, Backup.OriginalMaterials[Idx].Get());
			}
		}
	}

	if (UMaterialInstanceDynamic* Overlay = Entry.OverlayMID.Get())
	{
		ActiveDynamicMaterials.RemoveSingle(Overlay);
	}
	if (UMaterialInstanceDynamic* Dissolve = Entry.DissolveMID.Get())
	{
		ActiveDynamicMaterials.RemoveSingle(Dissolve);
	}

	Entry.OverlayMID.Reset();
	Entry.DissolveMID.Reset();
	Entry.MeshBackups.Reset();
	Entry.bWarningActive = false;
	Entry.bDissolveActive = false;
}

void USpectralObjectsSubsystem::RestoreAllVisuals()
{
	for (TPair<TWeakObjectPtr<AActor>, FRevealedEntry>& Pair : RevealedEntries)
	{
		RestoreActorVisuals(Pair.Key.Get(), Pair.Value);
	}
	RevealedEntries.Reset();
	ActiveDynamicMaterials.Reset();
}

void USpectralObjectsSubsystem::ActivateSpectralObjectsInRange(const FVector& Origin, const float Radius, const float Duration)
{
	UWorld* World = GetWorld();
	if (!World) return;

	EnsureVisualAssetsLoaded();

	const double Now = World->GetTimeSeconds();
	const double ExpireTime = Now + Duration;

	TArray<TWeakObjectPtr<AActor>> Candidates;
	Candidates.Reserve(SpectralObjects.Num());
	for (const TScriptInterface<ISpectral>& SpectralObject : SpectralObjects)
	{
		if (AActor* Actor = Cast<AActor>(SpectralObject.GetObject()))
		{
			Candidates.Add(Actor);
		}
	}

	for (const TWeakObjectPtr<AActor>& Candidate : Candidates)
	{
		AActor* Actor = Candidate.Get();
		if (!IsValid(Actor) || Actor->GetComponentsBoundingBox(true).ComputeSquaredDistanceToPoint(Origin) > FMath::Square(Radius)) continue;

		if (FRevealedEntry* Existing = RevealedEntries.Find(Actor))
		{
			Existing->ExpireTime = FMath::Max(Existing->ExpireTime, ExpireTime);

			if (Existing->bWarningActive || Existing->bDissolveActive)
			{
				RestoreActorVisuals(Actor, *Existing);
				Existing->PulsePhase = 0.f;
				Existing->LastTickTime = Now;
				ISpectral::Execute_OnSpectralStabilityChanged(Actor, 1.f, false);
			}
		}
		else
		{
			ISpectral::Execute_ActivateVisibility(Actor);

			FRevealedEntry& Entry = RevealedEntries.Add(Actor);
			Entry.ExpireTime = ExpireTime;
			Entry.LastTickTime = Now;
			ISpectral::Execute_OnSpectralStabilityChanged(Actor, 1.f, false);
		}
	}

	EnsureUpdateTimer();
}

bool USpectralObjectsSubsystem::IsActorRevealed(const AActor* Actor) const
{
	if (!Actor || !GetWorld()) return false;

	const FRevealedEntry* Entry = RevealedEntries.Find(const_cast<AActor*>(Actor));
	return Entry && Entry->ExpireTime > GetWorld()->GetTimeSeconds();
}

int32 USpectralObjectsSubsystem::GetRevealedObjectCount() const
{
	int32 Count = 0;
	for (const TPair<TWeakObjectPtr<AActor>, FRevealedEntry>& Pair : RevealedEntries)
	{
		if (Pair.Key.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

void USpectralObjectsSubsystem::UpdateRevealedObjects()
{
	UWorld* World = GetWorld();
	if (!World) return;

	EnsureVisualAssetsLoaded();

	const double Now = World->GetTimeSeconds();

	TArray<TWeakObjectPtr<AActor>> Keys;
	RevealedEntries.GetKeys(Keys);

	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	// Keys are re-resolved because callbacks may mutate the map.
	for (const TWeakObjectPtr<AActor>& Key : Keys)
	{
		FRevealedEntry* Entry = RevealedEntries.Find(Key);
		if (!Entry) continue;

		AActor* Actor = Key.Get();
		if (!IsValid(Actor))
		{
			RestoreActorVisuals(nullptr, *Entry);
			RevealedEntries.Remove(Key);
			continue;
		}

		const float DeltaTime = FMath::Clamp(static_cast<float>(Now - Entry->LastTickTime), 0.f, 0.1f);
		Entry->LastTickTime = Now;

		const double Remaining = Entry->ExpireTime - Now;

		if (Remaining <= 0.0)
		{
			const FBox Bounds = Actor->GetComponentsBoundingBox(true);
			const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : Actor->GetActorLocation();
			const FVector BoxSize = Bounds.IsValid ? Bounds.GetSize().ComponentMax(FVector(40.f, 40.f, 40.f)) : FVector(120.f, 120.f, 40.f);

			RestoreActorVisuals(Actor, *Entry);

			if (VanishNiagaraSystem)
			{
				if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, VanishNiagaraSystem, Center, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease))
				{
					NiagaraComp->SetVariableVec3(FName(TEXT("User.BoxSize")), BoxSize);
					NiagaraComp->Activate(true);
				}
			}

			if (VanishSound && Now - LastVanishSoundTime >= Spectral::VanishSoundCooldown)
			{
				const bool bInRange = !Player || FVector::DistSquared(Player->GetActorLocation(), Center) <= FMath::Square(Spectral::VanishSoundMaxDist);
				if (bInRange)
				{
					UGameplayStatics::PlaySoundAtLocation(World, VanishSound, Center, 0.42f, FMath::FRandRange(1.12f, 1.32f), 0.f, VanishAttenuation);
					LastVanishSoundTime = Now;
				}
			}

			RevealedEntries.Remove(Key);
			ISpectral::Execute_OnSpectralStabilityChanged(Actor, 0.f, false);
			ISpectral::Execute_DeactivateVisibility(Actor);
		}
		else if (Remaining <= Spectral::DissolveDuration)
		{
			const float DecayProgress = FMath::Clamp(1.f - static_cast<float>(Remaining / Spectral::WarningTime), 0.f, 1.f);
			Entry->PulsePhase += DeltaTime * FMath::Lerp(10.f, 38.f, DecayProgress * DecayProgress);

			const float RawDissolve = FMath::Clamp(1.f - static_cast<float>(Remaining / Spectral::DissolveDuration), 0.f, 1.f);
			const float DissolveAmount = FMath::InterpEaseIn(0.f, 1.f, RawDissolve, 1.45f);

			if (!Entry->bDissolveActive && DissolveMaterial)
			{
				CaptureMeshBackups(Actor, *Entry);

				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(DissolveMaterial, this);
				Entry->DissolveMID = MID;
				ActiveDynamicMaterials.Add(MID);

				for (const FMeshVisualBackup& Backup : Entry->MeshBackups)
				{
					if (UMeshComponent* Mesh = Backup.Mesh.Get())
					{
						Mesh->SetOverlayMaterial(Backup.PreviousOverlay.Get());
						const int32 NumMaterials = Mesh->GetNumMaterials();
						for (int32 Idx = 0; Idx < NumMaterials; ++Idx)
						{
							Mesh->SetMaterial(Idx, MID);
						}
					}
				}
				Entry->bDissolveActive = true;
			}

			if (UMaterialInstanceDynamic* MID = Entry->DissolveMID.Get())
			{
				MID->SetScalarParameterValue(TEXT("DissolveAmount"), DissolveAmount);
				MID->SetScalarParameterValue(TEXT("PulsePhase"), Entry->PulsePhase);
			}

			ISpectral::Execute_OnSpectralStabilityChanged(Actor, FMath::Clamp(static_cast<float>(Remaining / Spectral::WarningTime), 0.f, 1.f), true);
		}
		else if (Remaining <= Spectral::WarningTime)
		{
			const float DecayProgress = FMath::Clamp(1.f - static_cast<float>(Remaining / Spectral::WarningTime), 0.f, 1.f);
			Entry->PulsePhase += DeltaTime * FMath::Lerp(7.f, 34.f, DecayProgress * DecayProgress);

			if (!Entry->bWarningActive && DecayOverlayMaterial)
			{
				CaptureMeshBackups(Actor, *Entry);

				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(DecayOverlayMaterial, this);
				Entry->OverlayMID = MID;
				ActiveDynamicMaterials.Add(MID);

				for (const FMeshVisualBackup& Backup : Entry->MeshBackups)
				{
					if (UMeshComponent* Mesh = Backup.Mesh.Get())
					{
						Mesh->SetOverlayMaterial(MID);
					}
				}
				Entry->bWarningActive = true;
			}

			if (UMaterialInstanceDynamic* MID = Entry->OverlayMID.Get())
			{
				MID->SetScalarParameterValue(TEXT("DecayProgress"), DecayProgress);
				MID->SetScalarParameterValue(TEXT("PulsePhase"), Entry->PulsePhase);
			}

			ISpectral::Execute_OnSpectralStabilityChanged(Actor, 1.f - DecayProgress, true);
		}
	}

	if (RevealedEntries.IsEmpty())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
}

void USpectralObjectsSubsystem::ActivateSpectralObjects()
{
	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->Execute_ActivateVisibility(SpectralObject.GetObject());
	}
}

void USpectralObjectsSubsystem::DeactivateSpectralObjects()
{
	RestoreAllVisuals();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}

	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->Execute_DeactivateVisibility(SpectralObject.GetObject());
	}
}

void USpectralObjectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DeactivateSpectralObjects();
}

void USpectralObjectsSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}
	RestoreAllVisuals();

	Super::Deinitialize();
}
