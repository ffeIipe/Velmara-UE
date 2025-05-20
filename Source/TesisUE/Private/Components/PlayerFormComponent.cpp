// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PlayerFormComponent.h"
#include "Interfaces/CharacterState.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Items/Weapons/Sword.h"
#include "SpectralMode/SpectralObject.h"
#include "EngineUtils.h"

//TODO: optimize the iterator that search for spectral objects, 
//that in a future may cause an fps drop

UPlayerFormComponent::UPlayerFormComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SpectralEffectTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpectralEffectTimeline"));

    static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> MaterialCollection(TEXT("/Game/BloodsenseV2/PC_BloodSenseV2"));
    if (MaterialCollection.Succeeded())
    {  
        BloodSenseMaterialCollection = MaterialCollection.Object;
    }
}

void UPlayerFormComponent::BeginPlay()
{
    Super::BeginPlay();

    CharacterStateInterface = Cast<ICharacterState>(GetOwner());

    CharacterStateComponent = CharacterStateInterface->Execute_GetCharacterStateComponent(GetOwner());

    CharacterStateComponent->SetCharacterForm(ECharacterForm::ECF_Human);

    if (SpectralCurve)
    {
        FOnTimelineFloat ProgressFunction;
        ProgressFunction.BindUFunction(this, FName("UpdateSpectralEffect"));
        SpectralEffectTimeline->AddInterpFloat(SpectralCurve, ProgressFunction);
    }
}

void UPlayerFormComponent::ToggleForm(bool CanToggle)
{
    if (CanToggle)
    {
        CharacterStateComponent->IsFormEqualToAny({ ECharacterForm::ECF_Human }) ? ApplySpectralEffects() : ApplyHumanEffects();
    }
    else
    {
        ApplyHumanEffects();
    }
}

void UPlayerFormComponent::ApplySpectralEffects()
{
    CharacterStateComponent->SetCharacterForm(ECharacterForm::ECF_Spectral);
    SpectralEffectTimeline->PlayFromStart();
    UGameplayStatics::PlaySound2D(GetWorld(), EnableSpectralModeSFX);

    //TODO: improve it with a subscription to an a static class
    for (TActorIterator<ASpectralObject> It(GetWorld()); It; ++It)
    {
        It->SetSpectralVisibility(true);
    }
}

void UPlayerFormComponent::ApplyHumanEffects()
{
    CharacterStateComponent->SetCharacterForm(ECharacterForm::ECF_Human);
    SpectralEffectTimeline->Reverse();
    UGameplayStatics::PlaySound2D(GetWorld(), DisableSpectralModeSFX);

    //TODO: improve it with a subscription to an a static class
    for (TActorIterator<ASpectralObject> It(GetWorld()); It; ++It)
    {
        It->SetSpectralVisibility(false);
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