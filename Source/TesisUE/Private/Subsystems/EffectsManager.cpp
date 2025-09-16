#include "Subsystems/EffectsManager.h"

#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "DataAssets/EffectsManagerData.h"
#include "Kismet/GameplayStatics.h"

void UEffectsManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Data = Cast<UEffectsManagerData>(StaticLoadObject(UEffectsManagerData::StaticClass(), nullptr,
	TEXT("/Game/Blueprints/DataAssets/DA_EffectsManager.DA_EffectsManager")));
	if (Data)
	{
		UE_LOG(LogTemp, Log, TEXT("EffectsManager Data Asset loaded successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load EffectsManager Data Asset. Please check the path."));
	}
	
	bHasActiveTickableEffects = false;
}

void UEffectsManager::Deinitialize()
{
	Super::Deinitialize();

	Data = nullptr;
}

void UEffectsManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	//have to get the camera ref trough iteration... I think it's a bad practice tho
	//TODO: must find a new way to get this ref soon.
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		FollowCamera = *It;
		CameraComponent = FollowCamera->GetCameraComponent();
		break;
	}
}

void UEffectsManager::Tick(const float DeltaTime)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Red, "Effects Manager is ticking");
	
	//Camera Zoom
	if (ZoomEffectParams.bIsActive && CameraComponent)
	{
		const float Alpha = ZoomEffectParams.GetAlpha(DeltaTime);
		CameraComponent->SetFieldOfView(FMath::Lerp(ZoomEffectParams.InitialFOV, ZoomEffectParams.TargetFOV, Alpha));
	}

	//Time Warp
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

	//Hit Stop
	if (HitStopEffectParams.bIsActive)
	{
		if (const UWorld* World = GetWorld())
		{
			const float CurveValue = HitStopEffectParams.GetAlpha(DeltaTime);
			World->GetWorldSettings()->SetTimeDilation(CurveValue);
		}
	}
	
	UpdateActiveTickableEffectsStatus();
}

TStatId UEffectsManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEffectsManager, STATGROUP_Tickables);
}

bool UEffectsManager::IsTickable() const //manages tick enable and disable
{
	return GetWorld() != nullptr && !GetWorld()->IsPaused() && bHasActiveTickableEffects;
}

void UEffectsManager::CameraShake(const ECameraShakePreset CameraShakePreset, const FVector& Epicenter)
{
	if (!Data) return;

	FCameraShakeType CameraShakeSettings;
	if (Data->CameraShakes.Contains(CameraShakePreset))
	{
		CameraShakeSettings = Data->CameraShakes[CameraShakePreset];
	}
	else
	{
		CameraShakeSettings = Data->CameraShakes[ECameraShakePreset::ECSP_None];
	}

	UGameplayStatics::PlayWorldCameraShake(this, CameraShakeSettings.ShakeClass, Epicenter, CameraShakeSettings.InnerRadius, CameraShakeSettings.OuterRadius);
}

void UEffectsManager::HitStop(const EHitStopPreset HitStopPreset)
{
	if (!Data) return;

	FHitStopType HitStopSettings;
	if (Data->HitStops.Contains(HitStopPreset))
	{
		HitStopSettings = Data->HitStops[HitStopPreset];
	}
	else
	{
		HitStopSettings = Data->HitStops[EHitStopPreset::EHSP_None];
	}
	
	HitStopEffectParams.StartHitStop(HitStopSettings.Duration, HitStopSettings.Curve);
	UpdateActiveTickableEffectsStatus();
}

void UEffectsManager::TimeWarp(const ETimeWarpPreset TimeWarpPreset)
{
	if (!Data) return;

	FTimeWarpType TimeWarpSettings;
	if (Data->TimeWarps.Contains(TimeWarpPreset))
	{
		TimeWarpSettings = Data->TimeWarps[TimeWarpPreset];
	}
	else
	{
		TimeWarpSettings = Data->TimeWarps[ETimeWarpPreset::ETWP_None];
	}
	
	TimeWarpEffectParams.StartWarp(TimeWarpSettings.Curve, TimeWarpSettings.Duration);
	UpdateActiveTickableEffectsStatus();
}

void UEffectsManager::CameraZoom(const ECameraZoomPreset CameraZoomPreset)
{
    if (!CameraComponent || ZoomEffectParams.bIsActive || !Data) return;

	FCameraZoomType CameraZoomSettings;
	if (Data->CameraZooms.Contains(CameraZoomPreset))
	{
		CameraZoomSettings = Data->CameraZooms[CameraZoomPreset];
	}
	else
	{
		CameraZoomSettings = Data->CameraZooms[ECameraZoomPreset::ECZP_None];
	}
	
    ZoomEffectParams.StartZoom(CameraZoomSettings.Curve, CameraZoomSettings.Duration, CameraComponent->FieldOfView);
    UpdateActiveTickableEffectsStatus();
}

void UEffectsManager::UpdateActiveTickableEffectsStatus()
{
	bHasActiveTickableEffects = ZoomEffectParams.bIsActive || TimeWarpEffectParams.bIsActive || HitStopEffectParams.bIsActive;
}