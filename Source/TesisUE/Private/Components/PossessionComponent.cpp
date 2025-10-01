#include "Components/PossessionComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/AttributeComponent.h"
#include "DataAssets/EntityData.h"
#include "Entities/Entity.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/AttributeProvider.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UPossessionComponent::UPossessionComponent(): OwnerUtils(nullptr), PlayerController(nullptr)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPossessionComponent::InitializeValues(const FPossessionData& PossessionData)
{
    ReleaseAndExecuteEnergyTax = PossessionData.ReleaseAndExecuteEnergyTax;
}

void UPossessionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerUtils = GetOwner();
    CharacterStateProvider = GetOwner();
    AttributeProvider = GetOwner();
    AnimatorProvider = GetOwner();
    WeaponProvider = GetOwner();
    
    check(OwnerUtils != nullptr);

    PlayerController = UGameplayStatics::GetPlayerController(this, 0);
}

void UPossessionComponent::AttemptPossession(AEntity* Victim)
{
    if (IsPossessing() || CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->IsModeEqualToAny({ECharacterModeStates::ECMS_Spectral}))
    {
        if (AttributeProvider->RequiresEnergy(10.f))
        {
            if (!Victim) return;

            Victim->GetPossessionComponent()->OnPossessionReceived(Cast<AEntity>(OwnerUtils.GetObject()));
            
            check(PlayerController != nullptr);
            PlayerController->Possess(Victim);

            CurrentlyPossessedEntity = Victim;
            CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetMode(ECharacterModeStates::ECMS_Possessing);
            
            GetOwner()->SetActorHiddenInGame(true);
            GetOwner()->SetActorEnableCollision(false);
            AnimatorProvider->PauseAnims(true);

            if (WeaponProvider)
            {
                if (WeaponProvider->Execute_GetCurrentWeapon(GetOwner()))
                {
                    WeaponProvider->Execute_GetCurrentWeapon(GetOwner())->EnableVisuals();
                }
            }

            if (OnPossessionAttemptSucceed.IsBound())
            {
                OnPossessionAttemptSucceed.Broadcast();
            }
        }
    }
    else if (OnPossessionAttemptFailed.IsBound()) OnPossessionAttemptFailed.Broadcast();
}

void UPossessionComponent::ReleasePossession()
{
    if (!IsPossessing() || !CurrentlyPossessedEntity) return;

    check(PlayerController != nullptr);
    PlayerController->Possess(Cast<APawn>(OwnerUtils.GetObject()));

    const FVector ReleaseLocation = CurrentlyPossessedEntity->GetTargetActorLocation() + CurrentlyPossessedEntity->GetActorForwardVector() * 100.f;
    
    GetOwner()->SetActorLocationAndRotation(ReleaseLocation, CurrentlyPossessedEntity->GetActorRotation(), true);
    GetOwner()->SetActorHiddenInGame(false);
    GetOwner()->SetActorEnableCollision(true);
    AnimatorProvider->PauseAnims(false);
    CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetMode(ECharacterModeStates::ECMS_Spectral);

    if (WeaponProvider)
    {
        if (WeaponProvider->Execute_GetCurrentWeapon(GetOwner()))
        {
            WeaponProvider->Execute_GetCurrentWeapon(GetOwner())->EnableVisuals();
        }
    }

    if (OnPossessionReleased.IsBound()) OnPossessionReleased.Broadcast();
    
    CurrentlyPossessedEntity->GetPossessionComponent()->ReleasingPossession();
    CurrentlyPossessedEntity = nullptr;
}

void UPossessionComponent::EjectPossessor()
{
    if (IsPossessed() && PossessedByEntity)
    {
        if (UPossessionComponent* PossessorComponent = PossessedByEntity->GetPossessionComponent())
        {
            PossessorComponent->ReleasePossession();

            if (OnPossessorEjected.IsBound()) OnPossessorEjected.Broadcast();
        }
    }
}

void UPossessionComponent::OnPossessionReceived(AEntity* NewPossessor)
{
    if (OnPossessed.IsBound()) OnPossessed.Broadcast();
    
    PossessedByEntity = NewPossessor;

    const float EnergyFromPossessor = NewPossessor->GetAttributeComponent()->GetEnergy();
    AttributeProvider->SetEnergy(EnergyFromPossessor);
}

void UPossessionComponent::ReleasingPossession()
{
    PossessedByEntity = nullptr;

    if (OnPossessorEjected.IsBound()) OnPossessorEjected.Broadcast();
}

void UPossessionComponent::EjectAndExecute()
{
    if (!IsPossessed() || !PossessedByEntity) return;

    if (PossessedByEntity->GetAttributeComponent()->RequiresEnergy(ReleaseAndExecuteEnergyTax))
    {
        if (OnPossessorExecutedMeAndEjected.IsBound()) OnPossessorExecutedMeAndEjected.Broadcast();

        AttributeProvider->IncreaseHealth(20.f);
        
        PossessedByEntity->GetAttributeComponent()->SetEnergy(
            PossessedByEntity->GetAttributeComponent()->GetEnergy() - ReleaseAndExecuteEnergyTax);

        PossessedByEntity->GetPossessionComponent()->ReleasePossession();
    }
}

void UPossessionComponent::TryReleasePossession()
{
    if (GetPossessedEntity())
    {
        ReleasePossession();
    }
}

AEntity* UPossessionComponent::FindPossessionVictim(const float PossessDistance, const float PossessRadius) const
{
    if (!PlayerController) return nullptr;

    FVector StartLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(StartLocation, ViewRotation);

    const FVector TraceStart = StartLocation;
    const FVector TraceEnd = TraceStart + ViewRotation.Vector() * PossessDistance;

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    FHitResult HitResult;
    const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
        GetWorld(),
        TraceStart,
        TraceEnd,
        PossessRadius,
        TraceTypeQuery4,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForDuration,
        HitResult,
        true
    );

    if (!bHit) return nullptr;

    if (const TScriptInterface<ICombatTargetInterface> Target = HitResult.GetActor())
    {
        if (Target->IsAlive() &&
            !Target->IsPossessed() && Target->IsLaunchable())
        {
            return Cast<AEntity>(Target.GetObject());
        }
    }

    return nullptr;
}
