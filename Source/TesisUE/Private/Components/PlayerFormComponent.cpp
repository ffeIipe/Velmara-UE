// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PlayerFormComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Items/Weapons/Sword.h"

UPlayerFormComponent::UPlayerFormComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentForm = EPlayerForm::EPF_Human;

    SpectralEffectTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpectralEffectTimeline"));

    static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> MaterialCollection(TEXT("/Game/BloodSense/PC_BloodSense"));
    if (MaterialCollection.Succeeded())
    {
        BloodSenseMaterialCollection = MaterialCollection.Object;
    }
}

void UPlayerFormComponent::BeginPlay()
{
    Super::BeginPlay();

    if (SpectralCurve)
    {
        FOnTimelineFloat ProgressFunction;
        ProgressFunction.BindUFunction(this, FName("UpdateSpectralEffect"));
        SpectralEffectTimeline->AddInterpFloat(SpectralCurve, ProgressFunction);
    }
}

void UPlayerFormComponent::ToggleForm(ASword* EquippedWeapon)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastTransformationTime < TransformationCooldown) return;
    
    LastTransformationTime = 0;

    if (CurrentForm == EPlayerForm::EPF_Human)
    {
        CurrentForm = EPlayerForm::EPF_Spectral;
        ApplySpectralEffects(EquippedWeapon);
    }
    else
    {
        CurrentForm = EPlayerForm::EPF_Human;
        ApplyHumanEffects(EquippedWeapon);
    }

    LastTransformationTime = CurrentTime;
}

void UPlayerFormComponent::ApplySpectralEffects(ASword* EquippedWeapon)
{
    Debug(1, FColor::Red, FString("Spectral Mode"), true);

    SpectralEffectTimeline->Reverse();

    if (EquippedWeapon)
    {
        EquippedWeapon->Enable(false);
    }   
}

void UPlayerFormComponent::ApplyHumanEffects(ASword* EquippedWeapon)
{
    Debug(1, FColor::Blue, FString("Human Mode"), true);

    SpectralEffectTimeline->PlayFromStart();

    if (EquippedWeapon)
    {
        EquippedWeapon->Enable(true);
    }
}

void UPlayerFormComponent::UpdateSpectralEffect(float Value)
{
    if (BloodSenseMaterialCollection)
    {
        UMaterialParameterCollectionInstance* MaterialInstance = GetWorld()->GetParameterCollectionInstance(BloodSenseMaterialCollection);
        if (MaterialInstance)
        {
            float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
            MaterialInstance->SetScalarParameterValue(FName("Alpha"), ClampedValue);
        }
    }
}

void UPlayerFormComponent::Debug(int32 Key, FColor Color, FString String, bool bNewerOnTop)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(Key, 1.f, Color, String, bNewerOnTop);
    }
}