#include "Components/PlayerFormComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Entities/Entity.h"
#include <Player/PlayerHeroController.h>

#include "Subsystems/SpectralObjectsSubsystem.h"


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

void UPlayerFormComponent::ToggleForm()
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

void UPlayerFormComponent::ApplySpectralEffects()
{
    if (APlayerHeroController* PlayerController = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        /*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("PlayerController: Set Generic Team ID to 0"));*/
        PlayerController->SetGenericTeamId(FGenericTeamId(0));
    }
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
}

void UPlayerFormComponent::ApplyHumanEffects()
{
    if (APlayerHeroController* PlayerController = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("PlayerController: Set Generic Team ID to 2"));*/
        PlayerController->SetGenericTeamId(FGenericTeamId(2));
    }
    
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
}

void UPlayerFormComponent::UpdateSpectralEffect(float Value)
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