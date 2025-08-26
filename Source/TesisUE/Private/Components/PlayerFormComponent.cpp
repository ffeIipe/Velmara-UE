#include "Components/PlayerFormComponent.h"
#include "Components/SpectralObjectComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PossessionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Items/Weapons/Sword.h"
#include "SpectralMode/SpectralObject.h"
#include "EngineUtils.h"
#include "Entities/Entity.h"
#include "Player/PlayerMain.h"
#include "GameFramework/Character.h"
#include <Player/PlayerHeroController.h>

//TODO: optimize the iterator that search for spectral objects, that in a future may cause an fps drop

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

    EntityOwner = Cast<AEntity>(GetOwner());
    EntityOwner->GetCharacterStateComponent()->SetCharacterForm(ECharacterForm::ECF_Human);

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

    if (EntityOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
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
    
    {
        //-------------------------------------------------------------
        //TODO: se puede mejorar con una suscripcion a una static class
        for (TActorIterator<ASpectralObject> It(GetWorld()); It; ++It)
        {
            if (!It->GetSpectralObjectComponent())return;

            It->GetSpectralObjectComponent()->SetSpectralVisibility(true);
        }
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

    {
        //---------------------------------------------------------
        //TODO: improve it with a subscription to an a static class
        for (TActorIterator<ASpectralObject> It(GetWorld()); It; ++It)
        {
            if (!It || !It->GetSpectralObjectComponent()) return;

            It->GetSpectralObjectComponent()->SetSpectralVisibility(false);
        }
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