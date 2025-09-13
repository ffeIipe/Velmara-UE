#include "Components/ChangeModeComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Entities/Entity.h"

#include "Interfaces/Weapon/WeaponInterface.h"
#include "Subsystems/SpectralObjectsSubsystem.h"


UChangeModeComponent::UChangeModeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SpectralEffectTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpectralEffectTimeline"));

    static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> MaterialCollection(TEXT("/Game/BloodsenseV2/PC_BloodSenseV2"));
    if (MaterialCollection.Succeeded())
    {  
        BloodSenseMaterialCollection = MaterialCollection.Object;
    }
}

void UChangeModeComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUtils = GetOwner();
    CharacterStateProvider = GetOwner();
    CharacterStateProvider->SetMode(ECharacterModeStates::ECMS_Human);

    if (SpectralCurve)
    {
        FOnTimelineFloat ProgressFunction;
        ProgressFunction.BindUFunction(this, FName("UpdateSpectralEffect"));
        SpectralEffectTimeline->AddInterpFloat(SpectralCurve, ProgressFunction);
    }
}

void UChangeModeComponent::ToggleForm()
{
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastTransformationTime < TransformationCooldown) return;

    if (CharacterStateProvider->IsModeStateEqualToAny({ECharacterModeStates::ECMS_Human}))
    {
        ApplySpectralEffects();
    }
    else
    {
        ApplyHumanEffects();
    }

    LastTransformationTime = CurrentTime;
}

void UChangeModeComponent::ApplySpectralEffects()
{
    SpectralEffectTimeline->PlayFromStart();
    UGameplayStatics::PlaySound2D(GetWorld(), EnableSpectralModeSFX);

    if (OnSpectralEffectApplied.IsBound())
    {
        OnSpectralEffectApplied.Broadcast();
    }
    
    if (USpectralObjectsSubsystem* SpectralObjectsSubsystem = GetWorld()->GetSubsystem<USpectralObjectsSubsystem>())
    {
        SpectralObjectsSubsystem->ActivateSpectralObjects();
    }

    if (const TScriptInterface<IWeaponProvider> WeaponProvider = GetOwner())
    {
        if (WeaponProvider && WeaponProvider->GetCurrentWeapon())
        {
            WeaponProvider->GetCurrentWeapon()->EnableVisuals(false);
        }
    }
}

void UChangeModeComponent::ApplyHumanEffects()
{
    SpectralEffectTimeline->Reverse();
    UGameplayStatics::PlaySound2D(GetWorld(), DisableSpectralModeSFX);
    
    if (OnHumanEffectApplied.IsBound())
    {
        OnHumanEffectApplied.Broadcast();
    }

    if (USpectralObjectsSubsystem* SpectralObjectsSubsystem = GetWorld()->GetSubsystem<USpectralObjectsSubsystem>())
    {
        SpectralObjectsSubsystem->DeactivateSpectralObjects();
    }

    if (const TScriptInterface<IWeaponProvider> WeaponProvider = GetOwner())
    {
        if (WeaponProvider && WeaponProvider->GetCurrentWeapon())
        {
            WeaponProvider->GetCurrentWeapon()->EnableVisuals(true);
        }
    }
}

void UChangeModeComponent::UpdateSpectralEffect(float Value)
{
    if (BloodSenseMaterialCollection)
    {
        if (UMaterialParameterCollectionInstance* MaterialInstance = GetWorld()->GetParameterCollectionInstance(BloodSenseMaterialCollection))
        {
            const float ClampedValue = FMath::Clamp(Value, 0.0f, 1.0f);
            MaterialInstance->SetScalarParameterValue(FName("Alpha"), ClampedValue);
        }
    }
}