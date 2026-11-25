#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Features/GlobalEffectsSystem/Data/EffectsManagerData.h"
#include "Features/GlobalEffectsSystem/Data/Types.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Features/GlobalEffectsSystem/Interfaces/EffectManagerProvider.h"
#include "EffectsManager.generated.h"

class UCameraShakeBase;
class UChargeZoomCameraModifier;

UCLASS()
class TESISUE_API UEffectsManager : public UWorldSubsystem, public FTickableGameObject, public IEffectManagerProvider
{
	GENERATED_BODY()

public:
	virtual void PlayGameplayEffect_Implementation(FGameplayTag EffectTag, FVector Location) override;
	
	UFUNCTION(BlueprintCallable)
	void CameraShake(FGameplayTag EffectTag, const FVector& Epicenter) const;

	UFUNCTION(BlueprintCallable)
	void CameraZoom(FGameplayTag CameraZoomTag);

	UFUNCTION(BlueprintCallable)
	void BeginHeldCameraZoom(FGameplayTag CameraZoomTag, float ZoomInDuration, UObject* Source);

	UFUNCTION(BlueprintCallable)
	void EndHeldCameraZoom(const UObject* Source);

	UFUNCTION(BlueprintCallable)
	void HitStop(FGameplayTag HitStopTag);

	UFUNCTION(BlueprintCallable)
	void TimeWarp(FGameplayTag TimeWarpTag);
	
private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;

	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	
	virtual TStatId GetStatId() const override;

	virtual bool IsTickableInEditor() const override { return false; }

	virtual bool IsTickable() const override;

	UPROPERTY(Transient)
	TObjectPtr<UEffectsManagerData> Data;
	
	UPROPERTY()
	FTimeWarpEffectParameters TimeWarpEffectParams;

	UPROPERTY()
	FHitStopEffectParameters HitStopEffectParams;

	bool bHasActiveTickableEffects;

	void UpdateActiveTickableEffectsStatus();

	UChargeZoomCameraModifier* FindOrAddZoomModifier() const;
};
