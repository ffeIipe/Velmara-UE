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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UPlayerFormComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UPlayerFormComponent();

    UFUNCTION(BlueprintCallable)
    void ToggleForm(bool CanToggle);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "SFX")
    USoundBase* EnableSpectralModeSFX;

    UPROPERTY(EditDefaultsOnly, Category = "SFX")
    USoundBase* DisableSpectralModeSFX;

    ICharacterState* CharacterStateInterface;

    UCharacterStateComponent* CharacterStateComponent;

    void ApplySpectralEffects();

    void PlayEquipMontage();

    void ApplyHumanEffects();

    void PlaySwordMontage();

    bool bIsSpectralActive = false;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UMaterialParameterCollection* BloodSenseMaterialCollection;

    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* SpectralEffectTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UCurveFloat* SpectralCurve;

    UFUNCTION()
    void UpdateSpectralEffect(float Value);

    class ACharacter* OwningCharacter;

    UPROPERTY(EditDefaultsOnly)
    UAnimMontage* EquipMontage;
};
