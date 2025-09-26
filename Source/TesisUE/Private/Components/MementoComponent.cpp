#include "Components/MementoComponent.h"
#include "Components/AttributeComponent.h"
#include "GameFramework/Actor.h"
#include <Entities/Entity.h>

#include "Items/Weapons/Weapon.h"

UMementoComponent::UMementoComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    InternalMementoState = FEntityMementoState();
}

void UMementoComponent::BeginPlay()
{
    Super::BeginPlay();

    CaptureOwnerState();
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

        if (bShouldSaveInventory)
        {
            if (UInventoryComponent* InventoryComp = Owner->GetComponentByClass<UInventoryComponent>())
            {
                for (int32 i = 0; i < StateToApply.InventorySlots.Num(); i++)
                {
                    FActorSpawnParameters SpawnParams;
                    {
                        SpawnParams.Owner = GetOwner();
                        SpawnParams.Instigator = Cast<APawn>(GetOwner());
                        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    }
                    AWeapon* WeaponToAdd = GetWorld()->SpawnActor<AWeapon>(StateToApply.InventorySlots[i],
                                                                           GetOwner()->GetActorLocation(),
                                                                           GetOwner()->GetActorRotation(), SpawnParams);
                    WeaponToAdd->SetUniqueSaveID(StateToApply.UniqueSaveID);
                    if (!InventoryComp->TryAddWeapon(WeaponToAdd))
                    {
                        WeaponToAdd->Destroy();
                    }
                    //InventoryComp->EquipWeaponFromSlot(StateToApply.ActiveSaveSlotIndex);
                }
            }
        }
        
        InternalMementoState = StateToApply;
    }
}

FEntityMementoState UMementoComponent::CaptureOwnerState()
{
    if (const AEntity* EntityOwner = Cast<AEntity>(GetOwner()))
    {
        InternalMementoState.OwnerClass = EntityOwner->GetClass();
        InternalMementoState.Transform = EntityOwner->GetActorTransform();
        InternalMementoState.UniqueSaveID = EntityOwner->GetUniqueSaveID();
        
        if (UAttributeComponent* AttrComp = EntityOwner->GetAttributeComponent())
        {
            InternalMementoState.Health = AttrComp->GetHealth();
            InternalMementoState.Energy = AttrComp->GetEnergy();
            InternalMementoState.bIsAlive = AttrComp->IsAlive();
        }

        if (bShouldSaveInventory)
        {
            if (UInventoryComponent* InventoryComp = EntityOwner->GetInventoryComponent())
            {
                for (TScriptInterface Weapon : InventoryComp->InventorySlots)
                {
                    if (Weapon)
                    {
                        InternalMementoState.InventorySlots.Add(Weapon.GetObject()->GetClass());
                    }
                }
            
                InternalMementoState.ActiveSaveSlotIndex = InventoryComp->EquippedSlotIndex;
            }
        }
    }
    return InternalMementoState;
}