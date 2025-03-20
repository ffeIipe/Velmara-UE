// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PlayerFormComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"

UPlayerFormComponent::UPlayerFormComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentForm = EPlayerForm::Human;
    TransformationCooldown = 3.0f;
    LastTransformationTime = 0.0f;

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

void UPlayerFormComponent::ToggleForm()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastTransformationTime < TransformationCooldown)
    {
        UE_LOG(LogTemp, Warning, TEXT("Aún no puedes transformarte."));
        return;
    }

    if (CurrentForm == EPlayerForm::Human)
    {
        CurrentForm = EPlayerForm::Spectral;
        ApplySpectralEffects();
    }
    else
    {
        CurrentForm = EPlayerForm::Human;
        ApplyHumanEffects();
    }

    LastTransformationTime = CurrentTime;
}

void UPlayerFormComponent::ApplySpectralEffects()
{
    Debug(1, FColor::Red, FString("Spectral Mode"), true);

    if (bIsSpectralActive)
    {
        SpectralEffectTimeline->Reverse();
    }
    else
    {
        SpectralEffectTimeline->Play();
    }
    bIsSpectralActive = !bIsSpectralActive;
}

void UPlayerFormComponent::ApplyHumanEffects()
{
    Debug(2, FColor::Blue, FString("Human Mode"), true);
}

void UPlayerFormComponent::UpdateSpectralEffect(float Value)
{
    if (BloodSenseMaterialCollection)
    {
        UMaterialParameterCollectionInstance* MaterialInstance = GetWorld()->GetParameterCollectionInstance(BloodSenseMaterialCollection);
        if (MaterialInstance)
        {
            MaterialInstance->SetScalarParameterValue(FName("Alpha"), Value);
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