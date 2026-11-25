#include "Features/GlobalEffectsSystem/Core/EffectsManager.h"

#include "Camera/PlayerCameraManager.h"
#include "Features/GlobalEffectsSystem/Core/ChargeZoomCameraModifier.h"
#include "Features/GlobalEffectsSystem/Core/EffectsTags.h"
#include "Features/GlobalEffectsSystem/Data/EffectsManagerData.h"
#include "Features/GlobalEffectsSystem/Settings/EffectsManagerSettings.h"
#include "Kismet/GameplayStatics.h"

void UEffectsManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UEffectsSystemSettings* Settings = GetDefault<UEffectsSystemSettings>(); Settings && !Settings->EffectsDataAsset.IsNull())
	{
		Data = Settings->EffectsDataAsset.LoadSynchronous();
		
		if (Data)
		{
			UE_LOG(LogTemp, Log, TEXT("EffectsManager: Data Asset loaded from Project Settings."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EffectsManager: Failed to load Data Asset from Project Settings."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EffectsManager: No Data Asset assigned in Project Settings > Game > Effects System."));
	}
	
	bHasActiveTickableEffects = false;
}

void UEffectsManager::Deinitialize()
{
	Super::Deinitialize();

	Data = nullptr;
}

void UEffectsManager::Tick(const float DeltaTime)
{
	if (HitStopEffectParams.bIsActive)
	{
		if (const UWorld* World = GetWorld())
		{
			const float CurveValue = HitStopEffectParams.GetAlpha(DeltaTime);
			World->GetWorldSettings()->SetTimeDilation(CurveValue);
		}
	}
	
	if (TimeWarpEffectParams.bIsActive)
	{
		if (const UWorld* World = GetWorld())
		{
			const float CurrentScale = TimeWarpEffectParams.GetAlpha(DeltaTime);
			World->GetWorldSettings()->SetTimeDilation(CurrentScale);
		}

		if (!TimeWarpEffectParams.bIsActive)
		{
			if (const UWorld* World = GetWorld())
			{
				const float FinalCurveValue = TimeWarpEffectParams.Curve->GetFloatValue(1.0f);
				World->GetWorldSettings()->SetTimeDilation(FinalCurveValue);
			}
		}
	}
	
	UpdateActiveTickableEffectsStatus();
}

TStatId UEffectsManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEffectsManager, STATGROUP_Tickables);
}

bool UEffectsManager::IsTickable() const
{
	return GetWorld() != nullptr && !GetWorld()->IsPaused() && bHasActiveTickableEffects;
}

void UEffectsManager::PlayGameplayEffect_Implementation(FGameplayTag EffectTag, FVector Location)
{
	IEffectManagerProvider::PlayGameplayEffect_Implementation(EffectTag, Location);

	UEffectsManager* EffectsSys = nullptr;
	
	if (const UWorld* World = GetWorld())
	{
		EffectsSys = World->GetSubsystem<UEffectsManager>();
	}
    
	if (!EffectsSys) return;
    
	if (EffectTag.MatchesTag(EffectsTags::CameraShake)) 
	{
		EffectsSys->CameraShake(EffectTag, Location);
	}
	else if (EffectTag.MatchesTag(EffectsTags::CameraZoom))
	{
		EffectsSys->CameraZoom(EffectTag);
	}
	else if (EffectTag.MatchesTag(EffectsTags::TimeWarp))
	{
		EffectsSys->TimeWarp(EffectTag);
	}
	else if (EffectTag.MatchesTag(EffectsTags::HitStop))
	{
		EffectsSys->HitStop(EffectTag);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayGameplayEffect: Not a recognizable or categorized Tag %s"), *EffectTag.ToString());
	}
}

void UEffectsManager::CameraShake(const FGameplayTag EffectTag, const FVector& Epicenter) const
{
	if (!Data) return;

	if (const FCameraShakeType* FoundConfig = Data->CameraShakes.Find(EffectTag))
	{
		if (FoundConfig->ShakeClass)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::White, "CameraShake executed...");
			UGameplayStatics::PlayWorldCameraShake(this, FoundConfig->ShakeClass, Epicenter, FoundConfig->InnerRadius, FoundConfig->OuterRadius, FoundConfig->Falloff);
		}
	}
}

void UEffectsManager::CameraZoom(const FGameplayTag CameraZoomTag)
{
	const FCameraZoomType* Config = Data ? Data->CameraZooms.Find(CameraZoomTag) : nullptr;
	if (!Config) return;

	if (UChargeZoomCameraModifier* Modifier = FindOrAddZoomModifier())
	{
		Modifier->StartZoom(*Config, Config->Duration, nullptr);
	}
}

void UEffectsManager::BeginHeldCameraZoom(const FGameplayTag CameraZoomTag, const float ZoomInDuration, UObject* Source)
{
	if (!Source) return;

	const FCameraZoomType* Config = Data ? Data->CameraZooms.Find(CameraZoomTag) : nullptr;
	if (!Config)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginHeldCameraZoom: No camera zoom configured for %s"), *CameraZoomTag.ToString());
		return;
	}

	if (UChargeZoomCameraModifier* Modifier = FindOrAddZoomModifier())
	{
		Modifier->StartZoom(*Config, ZoomInDuration, Source);
	}
}

void UEffectsManager::EndHeldCameraZoom(const UObject* Source)
{
	if (UChargeZoomCameraModifier* Modifier = FindOrAddZoomModifier())
	{
		Modifier->ReleaseZoom(Source);
	}
}

UChargeZoomCameraModifier* UEffectsManager::FindOrAddZoomModifier() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager.Get() : nullptr;
	if (!CameraManager) return nullptr;

	if (UCameraModifier* Existing = CameraManager->FindCameraModifierByClass(UChargeZoomCameraModifier::StaticClass()))
	{
		return Cast<UChargeZoomCameraModifier>(Existing);
	}

	return Cast<UChargeZoomCameraModifier>(CameraManager->AddNewCameraModifier(UChargeZoomCameraModifier::StaticClass()));
}

void UEffectsManager::HitStop(const FGameplayTag HitStopTag)
{
	if (!Data) return;

	if (const FHitStopType* FoundConfig = Data->HitStops.Find(HitStopTag))
	{
		HitStopEffectParams.StartHitStop(FoundConfig->Duration, FoundConfig->Curve);
		UpdateActiveTickableEffectsStatus();
	}
}

void UEffectsManager::TimeWarp(const FGameplayTag TimeWarpTag)
{
	if (!Data) return;

	if (const FTimeWarpType* FoundConfig = Data->TimeWarps.Find(TimeWarpTag))
	{
		TimeWarpEffectParams.StartWarp(FoundConfig->Curve, FoundConfig->Duration);
		UpdateActiveTickableEffectsStatus();
	}
}

void UEffectsManager::UpdateActiveTickableEffectsStatus()
{
	bHasActiveTickableEffects = TimeWarpEffectParams.bIsActive || HitStopEffectParams.bIsActive;
}