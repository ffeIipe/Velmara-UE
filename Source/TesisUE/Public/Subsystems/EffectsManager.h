#pragma once

#include "CoreMinimal.h"
#include "DataAssets/EffectsManagerData.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "EffectsManager.generated.h"

class UCameraShakeBase;
class UCameraComponent;

USTRUCT()
struct FEffectProgressParameters
{
    GENERATED_BODY()

    UPROPERTY()
    float CurrentTime;

    UPROPERTY()
    float Duration;

    UPROPERTY()
    bool bIsActive;

	UPROPERTY()
	UCurveFloat* Curve;
	
    FEffectProgressParameters()
        : CurrentTime(0.0f)
        , Duration(0.0f)
        , bIsActive(false)
		, Curve(nullptr)
    {}

    void Start(const float InDuration)
    {
        Duration = FMath::Max(KINDA_SMALL_NUMBER, InDuration);
        CurrentTime = 0.0f;
        bIsActive = true;
    }

    void Stop()
    {
        bIsActive = false;
        CurrentTime = 0.0f;
    }
	
    float GetAlpha(const float DeltaTime)
    {
        if (!bIsActive)
        {
            return 0.0f;
        }

        CurrentTime += DeltaTime;
    	const float Alpha = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);
    	
        if (Alpha >= 1.0f)
        {
            Stop();
        }

    	if (Curve != nullptr)
    	{
    		const float CurvedAlpha = Curve->GetFloatValue(Alpha);
    		return CurvedAlpha;
    	}
    	
        return Alpha;
    }
};

USTRUCT()
struct FZoomEffectParameters : public FEffectProgressParameters
{
    GENERATED_BODY()

    UPROPERTY()
    float InitialFOV;

    UPROPERTY()
    float TargetFOV;

    FZoomEffectParameters()
        : InitialFOV(90.0f)
        , TargetFOV(0.0f)
    {}

    void StartZoom(UCurveFloat* InCurveAsset, const float InDuration, const float InInitialFOV)
    {
        Start(InDuration);
    	Curve = InCurveAsset;
        InitialFOV = InInitialFOV;
        // TargetFOV = InTargetFOV;
    }
};

USTRUCT()
struct FTimeWarpEffectParameters : public FEffectProgressParameters
{
	GENERATED_BODY()
	
	UPROPERTY()
	float MinTimeScaleValue; 

	UPROPERTY()
	float MaxTimeScaleValue; 

	FTimeWarpEffectParameters()
		: MinTimeScaleValue(0.0f)
		, MaxTimeScaleValue(1.0f)
	{}

	void StartWarp(UCurveFloat* InCurveAsset, const float InDuration)
	{
		Start(InDuration);
		
		if (InCurveAsset)
		{
			Curve = InCurveAsset;
		}
	}
};

USTRUCT()
struct FHitStopEffectParameters : public FEffectProgressParameters
{
	GENERATED_BODY()

	UPROPERTY()
	float MinTimeScaleValue;

	UPROPERTY()
	float MaxTimeScaleValue;

	FHitStopEffectParameters()
	: MinTimeScaleValue(0.f)
	, MaxTimeScaleValue(1.0f)
	{}

	void StartHitStop(const float InDuration, UCurveFloat* InCurveAsset)
	{
		Start(InDuration);
		
		if (InCurveAsset)
		{
			Curve = InCurveAsset;
		}
	}
};

UCLASS()
class TESISUE_API UEffectsManager : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Camera Shake */
	UFUNCTION(BlueprintCallable)
	void CameraShake(ECameraShakePreset CameraShakePreset, const FVector& Epicenter);

	/** Camera Zoom */
	UFUNCTION(BlueprintCallable)
	void CameraZoom(ECameraZoomPreset CameraZoomPreset);

	/** Hit Stop */
	UFUNCTION(BlueprintCallable)
	void HitStop(EHitStopPreset HitStopPreset);

	/** Time Acceleration & Deceleration */
	UFUNCTION(BlueprintCallable)
	void TimeWarp(ETimeWarpPreset TimeWarpPreset);
	
private:
	/** Initialization */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	virtual void Tick(float DeltaTime) override;

	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	
	virtual TStatId GetStatId() const override;

	virtual bool IsTickableInEditor() const override { return false; }

	virtual bool IsTickable() const override;

	/** Refs */
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> FollowCamera;
	
	UPROPERTY(Transient)
	TObjectPtr<UEffectsManagerData> Data;
	
	/** Variables */
	UPROPERTY()
	FZoomEffectParameters ZoomEffectParams;

	UPROPERTY()
	FTimeWarpEffectParameters TimeWarpEffectParams;

	UPROPERTY()
	FHitStopEffectParameters HitStopEffectParams;

	bool bHasActiveTickableEffects;

	void UpdateActiveTickableEffectsStatus();
};
