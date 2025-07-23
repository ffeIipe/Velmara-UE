// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EffectsManagerData.generated.h"

class UCurveFloat;

USTRUCT(BlueprintType)
struct FCameraShakeType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake")
	TSubclassOf<UCameraShakeBase> ShakeClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta=(UIMin="0.0", ClampMin="0.0"))
	float InnerRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta=(UIMin="0.0", ClampMin="0.0"))
	float OuterRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Shake", meta=(UIMin="0.0", ClampMin="0.0"))
	float Falloff;
	
	FCameraShakeType()
		: ShakeClass(nullptr)
		, InnerRadius(0.0f)
		, OuterRadius(500.0f)
		, Falloff(1.0f)
	{}
	
	FCameraShakeType(const TSubclassOf<UCameraShakeBase>& InShakeClass, const float InInnerRadius, const float InOuterRadius, const float InFalloff = 1.0f)
		: ShakeClass(InShakeClass)
		, InnerRadius(InInnerRadius)
		, OuterRadius(InOuterRadius)
		, Falloff(InFalloff)
	{}
};

UENUM(BlueprintType)
enum class ECameraShakePreset : uint8
{
	ECSP_Default UMETA(DisplayName = "Default"),
	ECSP_ShotHit UMETA(DisplayName = "Shot Hit"),
	ECSP_SwordHit UMETA(DisplayName = "Sword Hit"),
	ECSP_HeavyHit UMETA(DisplayName = "Heavy Hit"),
	ECSP_Explosion UMETA(DisplayName = "Explosion"),
};

USTRUCT(BlueprintType)
struct FEffectBaseType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UCurveFloat* Curve;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float Duration;

	FEffectBaseType() : Curve(nullptr), Duration(0.0f) {}

	FEffectBaseType(UCurveFloat* InCurve, const float InDuration) : Curve(InCurve), Duration(InDuration) {}
};

USTRUCT(BlueprintType)
struct FCameraZoomType : public FEffectBaseType
{
	GENERATED_BODY()
	
	FCameraZoomType() : FEffectBaseType(nullptr, 0.0f) {}

	FCameraZoomType(UCurveFloat* CurveAsset, const float Duration) : FEffectBaseType(CurveAsset, Duration) {}
};

UENUM(BlueprintType)
enum class ECameraZoomPreset : uint8
{
	ECZP_Default UMETA(DisplayName = "Default"),
	ECZP_Finisher UMETA(DisplayName = "Finisher")
};

USTRUCT(BlueprintType)
struct FTimeWarpType : public FEffectBaseType
{
	GENERATED_BODY()

	FTimeWarpType() : FEffectBaseType(nullptr, 0.0f) {}

	FTimeWarpType(UCurveFloat* CurveAsset, const float Duration) : FEffectBaseType(CurveAsset, Duration) {}
};

UENUM(BlueprintType)
enum class ETimeWarpPreset : uint8
{
	ETWP_Default UMETA(DisplayName = "Default"),
	ETWP_Finisher UMETA(DisplayName = "Finisher"),
	ETWP_Crasher UMETA(DisplayName = "Crasher"),
	ETWP_SwitchForm UMETA(DisplayName = "Switch Form")
};

USTRUCT(BlueprintType)
struct FHitStopType : public FEffectBaseType
{
	GENERATED_BODY()

	FHitStopType() : FEffectBaseType(nullptr, 0.0f) {}

	FHitStopType(UCurveFloat* InCurveAsset, const float Duration) : FEffectBaseType(InCurveAsset, Duration) {}
};

UENUM(BlueprintType)
enum class EHitStopPreset : uint8
{
	EHSP_Default UMETA(DisplayName = "Default"),
	EHSP_SwordHit UMETA(DisplayName = "Sword Hit"),
	EHSP_HeavyHit UMETA(DisplayName = "Heavy Hit"),
	EHSP_Finisher UMETA(DisplayName = "Finisher")
};

UCLASS()
class TESISUE_API UEffectsManagerData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Shake")
	TMap<ECameraShakePreset, FCameraShakeType> CameraShakes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Zoom")
	TMap<ECameraZoomPreset, FCameraZoomType> CameraZooms;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Warp")
	TMap<ETimeWarpPreset, FTimeWarpType> TimeWarps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Stop")
	TMap<EHitStopPreset, FHitStopType> HitStops;
};
