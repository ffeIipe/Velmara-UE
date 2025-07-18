#include "Components/MementoComponent.h"
#include "Components/AttributeComponent.h"
#include "GameFramework/Actor.h"
#include <Entities/Entity.h>

UMementoComponent::UMementoComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    InternalMementoState = FEntityMementoState();
}

void UMementoComponent::BeginPlay()
{
    Super::BeginPlay();

    SaveState();
}

void UMementoComponent::SaveState()
{
    if (GetOwner())
    {
        InternalMementoState = CaptureOwnerState();
    }
}

void UMementoComponent::LoadState()
{
    if (GetOwner())
    {
        ApplyExternalState(InternalMementoState);
    }
}

void UMementoComponent::ApplyExternalState(const FEntityMementoState& StateToApply)
{
    if (AActor* Owner = GetOwner())
    {
        Owner->SetActorTransform(StateToApply.Transform);
        if (UAttributeComponent* AttrComp = Owner->GetComponentByClass<UAttributeComponent>())
        {
            AttrComp->SetHealth(StateToApply.Health);
            AttrComp->SetEnergy(StateToApply.Energy);
        }

        InternalMementoState = StateToApply;
    }
}

FEntityMementoState UMementoComponent::CaptureOwnerState() const
{
    FEntityMementoState State;

    if (AEntity* EntityOwner = Cast<AEntity>(GetOwner()))
    {
        State.Transform = EntityOwner->GetActorTransform();
        if (UAttributeComponent* AttrComp = EntityOwner->GetAttributeComponent())
        {
            State.Health = AttrComp->GetHealth();
            State.Energy = AttrComp->GetEnergy();
        }
        else
        {
            State.Health = 0.0f;
            State.Energy = 0.0f;
        }
    }
    return State;
}