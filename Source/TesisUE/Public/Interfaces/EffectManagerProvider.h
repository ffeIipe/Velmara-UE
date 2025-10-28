// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/EffectsManagerData.h"
#include "UObject/Interface.h"
#include "EffectManagerProvider.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UEffectManagerProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IEffectManagerProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/*DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCameraShake, ECameraShakePreset, CameraShakePreset, const FVector&, ImpactPoint);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraZoom, ECameraZoomPreset, CameraZoomPreset);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeWarp, ETimeWarpPreset, TimeWarpPreset);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitStop, EHitStopPreset, HitStopPreset);*/
};
