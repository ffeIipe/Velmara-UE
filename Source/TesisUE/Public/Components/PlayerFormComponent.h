// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CharacterStates.h"
#include "PlayerFormComponent.generated.h"

class UMaterialParameterCollection;
class UTimelineComponent;
class UCurveFloat;
class ASword;
class ICharacterState;
class UCharacterStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHumanEffectApplied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpectralEffectApplied);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UPlayerFormComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerFormComponent();

    UFUNCTION(BlueprintCallable)
    void ToggleForm();

    FOnHumanEffectApplied OnHumanEffectApplied;
    FOnSpectralEffectApplied OnSpectralEffectApplied;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "SFX")
    USoundBase* EnableSpectralModeSFX;

    UPROPERTY(EditDefaultsOnly, Category = "SFX")
    USoundBase* DisableSpectralModeSFX;

    UPROPERTY(EditDefaultsOnly, Category = "Stats | Forms | Cooldown")
    float TransformationCooldown = 1.f;

    UPROPERTY(EditAnywhere, Category = "State | Forms | Cooldown")
    float LastTransformationTime;

    void ApplySpectralEffects();

    void ApplyHumanEffects();

    bool bIsSpectralActive = false;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UMaterialParameterCollection* BloodSenseMaterialCollection;

    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* SpectralEffectTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UCurveFloat* SpectralCurve;

    UFUNCTION()
    void UpdateSpectralEffect(float Value);

    class AEntity* EntityOwner;

    UPROPERTY(EditDefaultsOnly)
    UAnimMontage* EquipMontage;
};
