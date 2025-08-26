#include "Components/PossessionComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/InventoryComponent.h"
#include "DataAssets/EntityData.h"
#include "Items/Item.h"
#include "Entities/Entity.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UPossessionComponent::UPossessionComponent(): OwnerEntity(nullptr), PlayerController(nullptr)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPossessionComponent::InitializeValues(const FPossessionData& PossessionData)
{
    PossessDistance = PossessionData.PossessDistance;
    PossessRadius = PossessionData.PossessRadius;
    ReleaseAndExecuteEnergyTax = PossessionData.ReleaseAndExecuteEnergyTax;
}

void UPossessionComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerEntity = Cast<AEntity>(GetOwner());
    check(OwnerEntity != nullptr);

    PlayerController = UGameplayStatics::GetPlayerController(this, 0);
}

void UPossessionComponent::AttemptPossession()
{
    if (IsPossessing() || OwnerEntity->GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
    {
        if (OwnerEntity->GetAttributeComponent()->RequiresEnergy(10.f))
        {
            AEntity* Victim = FindPossessionVictim();
            if (!Victim) return;

            Victim->GetPossessionComponent()->OnPossessionReceived(OwnerEntity);

            check(PlayerController != nullptr);
            PlayerController->Possess(Victim);

            CurrentlyPossessedEntity = Victim;
            OwnerEntity->GetCharacterStateComponent()->SetCharacterForm(ECharacterForm::ECF_Possessing);
            OwnerEntity->AttachFollowCamera(Victim->GetSpringArmComponent());
            OwnerEntity->SetActorHiddenInGame(true);
            OwnerEntity->SetActorEnableCollision(false);
            OwnerEntity->GetMesh()->bPauseAnims = true;

            if (OwnerEntity->GetInventoryComponent()->GetEquippedItem())
            {
                OwnerEntity->GetInventoryComponent()->GetEquippedItem()->SetActorHiddenInGame(true);
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
    PlayerController->Possess(OwnerEntity);

    const FVector ReleaseLocation = CurrentlyPossessedEntity->GetActorLocation() + CurrentlyPossessedEntity->GetActorForwardVector() * 100.f;
    
    OwnerEntity->AttachFollowCamera(OwnerEntity->GetSpringArmComponent());
    OwnerEntity->SetActorLocationAndRotation(ReleaseLocation, CurrentlyPossessedEntity->GetActorRotation(), true);
    OwnerEntity->SetActorHiddenInGame(false);
    OwnerEntity->SetActorEnableCollision(true);
    OwnerEntity->GetMesh()->bPauseAnims = false;
    OwnerEntity->GetCharacterStateComponent()->SetCharacterForm(ECharacterForm::ECF_Spectral);

    if (OwnerEntity->GetInventoryComponent()->GetEquippedItem())
    {
        OwnerEntity->GetInventoryComponent()->GetEquippedItem()->SetActorHiddenInGame(false);
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
    OwnerEntity->GetAttributeComponent()->SetEnergy(EnergyFromPossessor);
    OwnerEntity->GetAttributeComponent()->StartDecreaseEnergy();
}

void UPossessionComponent::ReleasingPossession()
{
    OwnerEntity->AttachFollowCamera(PossessedByEntity->GetSpringArmComponent());
    PossessedByEntity = nullptr;

    if (OnPossessorEjected.IsBound()) OnPossessorEjected.Broadcast();
}

void UPossessionComponent::EjectAndExecute()
{
    if (!IsPossessed() || !PossessedByEntity) return;

    if (PossessedByEntity->GetAttributeComponent()->RequiresEnergy(ReleaseAndExecuteEnergyTax))
    {
        OwnerEntity->GetAttributeComponent()->IncreaseHealth(20.f);
        
        PossessedByEntity->GetAttributeComponent()->SetEnergy(
            PossessedByEntity->GetAttributeComponent()->GetEnergy() - ReleaseAndExecuteEnergyTax);

        PossessedByEntity->GetPossessionComponent()->ReleasePossession();

        if (OnPossessorExecutedMeAndEjected.IsBound()) OnPossessorExecutedMeAndEjected.Broadcast();
    }
}

AEntity* UPossessionComponent::FindPossessionVictim() const
{
    if (!PlayerController) return nullptr;

    FVector StartLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(StartLocation, ViewRotation);

    const FVector TraceStart = StartLocation;
    const FVector TraceEnd = TraceStart + ViewRotation.Vector() * PossessDistance;

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerEntity);

    FHitResult HitResult;
    const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
        GetWorld(),
        TraceStart,
        TraceEnd,
        PossessRadius,
        ETraceTypeQuery::TraceTypeQuery4,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        HitResult,
        true
    );

    if (!bHit) return nullptr;

    if (AEntity* HitEntity = Cast<AEntity>(HitResult.GetActor()); HitEntity && HitEntity->GetPossessionComponent())
    {
        if (HitEntity->GetCharacterStateComponent()->GetCurrentCharacterState().Action != ECharacterActions::ECA_Dead &&
            !HitEntity->GetPossessionComponent()->IsPossessed() && HitEntity->IsLaunchable_Implementation())
        {
            return HitEntity;
        }
    }

    return nullptr;
}