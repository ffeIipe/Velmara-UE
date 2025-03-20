// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerFormComponent.generated.h"

class UMaterialParameterCollection;
class UTimelineComponent;
class UCurveFloat;

UENUM(BlueprintType)
enum class EPlayerForm : uint8
{
	Human UMETA(DisplayName = "Human"),
	Spectral UMETA(DisplayName = "Spectral")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UPlayerFormComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UPlayerFormComponent();

    UFUNCTION(BlueprintCallable)
    void ToggleForm();

    EPlayerForm GetCurrentForm() const { return CurrentForm; }

protected:
    virtual void BeginPlay() override;

private:
    void ApplySpectralEffects();
    void ApplyHumanEffects();

    EPlayerForm CurrentForm;
    float TransformationCooldown;
    float LastTransformationTime;

    bool bIsSpectralActive = false;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UMaterialParameterCollection* BloodSenseMaterialCollection;

    UTimelineComponent* SpectralEffectTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "BloodSense")
    UCurveFloat* SpectralCurve;

    UFUNCTION()
    void UpdateSpectralEffect(float Value);

    void Debug(int32 Key, FColor Color, FString String, bool bNewerOnTop);
};
