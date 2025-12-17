#include "Components/PossessionComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "Entities/Entity.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UPossessionComponent::UPossessionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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
    if (IsPossessing() || OwnerEntity->GetCharacterStateComponent()->GetCurrentCharacterState().Form != ECharacterForm::ECF_Spectral)
    {
        return;
    }

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

    CurrentlyPossessedEntity->GetPossessionComponent()->OnPossessionReleased();
    CurrentlyPossessedEntity = nullptr;
}

void UPossessionComponent::EjectPossessor()
{
    if (IsPossessed() && PossessedByEntity)
    {
        UPossessionComponent* PossessorComponent = PossessedByEntity->GetPossessionComponent();
        if (PossessorComponent)
        {
            PossessorComponent->ReleasePossession();

            if (OnPossessorEjected.IsBound()) OnPossessorEjected.Broadcast();
        }
    }
}

void UPossessionComponent::OnPossessionReceived(AEntity* NewPossessor)
{
    PossessedByEntity = NewPossessor;

    const float EnergyFromPossessor = NewPossessor->GetAttributeComponent()->GetEnergy();
    OwnerEntity->GetAttributeComponent()->SetEnergy(EnergyFromPossessor);
    OwnerEntity->GetAttributeComponent()->StartDecreaseEnergy();

    if (OnPossessed.IsBound()) OnPossessed.Broadcast();
}

void UPossessionComponent::OnPossessionReleased()
{
    OwnerEntity->AttachFollowCamera(PossessedByEntity->GetSpringArmComponent());
    PossessedByEntity = nullptr;

    OwnerEntity->GetAttributeComponent()->StopDecreaseEnergy();

    if (OnPossessorEjected.IsBound()) OnPossessorEjected.Broadcast();
}

void UPossessionComponent::EjectAndExecute()
{
    if (!IsPossessed() || !PossessedByEntity) return;

    if (PossessedByEntity->GetAttributeComponent()->RequiresEnergy(ReleaseAndExecuteEnergyTax))
    {
        OwnerEntity->GetAttributeComponent()->IncreaseHealth(15.f);
        PossessedByEntity->GetAttributeComponent()->IncreaseEnergy(-ReleaseAndExecuteEnergyTax);

        PossessedByEntity->GetPossessionComponent()->ReleasePossession();

        if (OnPossessorExecutedMeAndEjected.IsBound()) OnPossessorExecutedMeAndEjected.Broadcast();
    }
}

AEntity* UPossessionComponent::FindPossessionVictim()
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
    bool bHit = UKismetSystemLibrary::SphereTraceSingle(
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

    AEntity* HitEntity = Cast<AEntity>(HitResult.GetActor());
    if (HitEntity && HitEntity->GetPossessionComponent())
    {
        if (HitEntity->GetCharacterStateComponent()->GetCurrentCharacterState().Action != ECharacterActions::ECA_Dead &&
            !HitEntity->GetPossessionComponent()->IsPossessed())
        {
            return HitEntity;
        }
    }

    return nullptr;
}