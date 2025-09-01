#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HUD/Inventory.h"

#include "DataAssets/EntityData.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/Weapon/WeaponInterface.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    InventorySlots.Init(nullptr, MaxSlots);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    InitializeInventoryWidget();
}

void UInventoryComponent::InitializeValues(const FInventoryData& InventoryData)
{
    MaxSlots = InventoryData.MaxSlots;
}

void UInventoryComponent::ChangeWeapon(int32 SlotIndex)
{
    if (InventorySlots.IsValidIndex(SlotIndex))
    {
        EquipWeaponFromSlot(SlotIndex);
    }
}

void UInventoryComponent::InitializeInventoryWidget()
{
    if (InventoryWidgetClass && PlayerControllerRef && PlayerControllerRef->IsLocalController() && !InventoryWidgetInstance)
    {
        InventoryWidgetInstance = CreateWidget<UInventory>(PlayerControllerRef, InventoryWidgetClass);
        if (InventoryWidgetInstance)
        {
            InventoryWidgetInstance->AddToViewport();
            InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

bool UInventoryComponent::TryAddWeapon(AItem* ItemToAdd)
{
    if (!IsValid(ItemToAdd))
    {
        return false;
    }

    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i] == nullptr)
        {
            InventorySlots[i] = ItemToAdd;

            ChangeWeapon(i);

            UpdateInventoryUI();
            return true;
        }
    }

    return false; // Inventario lleno
}

void UInventoryComponent::EquipWeaponFromSlot(int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr) return;

    TScriptInterface<IWeaponInterface> ItemToEquip = InventorySlots[SlotIndex];
    UnequipCurrentWeapon();

    if (ItemToEquip)
    {
        if (const TScriptInterface<IAnimatorProvider> AnimatorProvider = GetOwner())
        {
            ItemToEquip->Equip(AnimatorProvider->GetMesh(), HandSocketName, GetOwner(), Cast<APawn>(GetOwner()));

            //ItemToEquip->EnableVisuals(true);

            EquippedWeapon = ItemToEquip;
            EquippedSlotIndex = SlotIndex;
        }
    }

    UpdateInventoryUI();
}

void UInventoryComponent::DropWeaponFromSlot(int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr)
    {
        return;
    }

    TScriptInterface<IWeaponInterface> ItemToDrop = InventorySlots[SlotIndex];
    ItemToDrop = EquippedWeapon;
    
    if (EquippedWeapon)
    {
        UnequipCurrentWeapon();
    }

    InventorySlots[SlotIndex] = nullptr;

    // if (GetOwner() && GetWorld() && ItemToDrop)
    // {
    //     ItemToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    //
    //     ItemToDrop->EnableVisuals(true);
    //
    //     FVector DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 150.0f + FVector(0, 0, 50.0f);
    //     FRotator DropRotation = GetOwner()->GetActorRotation();
    //
    //     ItemToDrop->TeleportTo(DropLocation, DropRotation, false, true);
    //     // ItemToDrop->ItemState = EItemState::EIS_Hovering;
    // }

    UpdateInventoryUI();
}

void UInventoryComponent::UnequipCurrentWeapon()
{
    if (EquippedWeapon)
    {
        //EquippedItem->EnableVisuals(false);
        //ASword* SwordRef = Cast<ASword>(EquippedItem)->Set
        EquippedWeapon = nullptr;
        EquippedSlotIndex = -1;
    }
    else
    {
        if (EquippedSlotIndex != -1)
        {
            EquippedSlotIndex = -1;
        }
    }
}

void UInventoryComponent::ShowInventory()
{
    if (bIsInventoryOpen) return;
    if (!InventoryWidgetInstance)
    {
        InitializeInventoryWidget();
        if (!InventoryWidgetInstance) return;
    }

    bIsInventoryOpen = true;
    UpdateInventoryUI();
    InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.2f);

    if (PlayerControllerRef)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerControllerRef->SetInputMode(InputMode);
        PlayerControllerRef->bShowMouseCursor = true;
    }
}

void UInventoryComponent::HideInventory()
{
    if (!bIsInventoryOpen) return;
    if (!InventoryWidgetInstance) return;

    bIsInventoryOpen = false;
    InventoryWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

    if (PlayerControllerRef)
    {
        FInputModeGameOnly InputMode;
        PlayerControllerRef->SetInputMode(InputMode);
        PlayerControllerRef->bShowMouseCursor = false;
    }
}

void UInventoryComponent::UpdateInventoryUI()
{
    if (InventoryWidgetInstance)
    {
        InventoryWidgetInstance->RefreshInventoryUI(InventorySlots);
    }
}

// void UInventoryComponent::Interact()
// {
//     if (IsInventoryOpen()) return;
//
//     FVector TraceStart;
//     FRotator CameraRotation;
//     Controller->GetPlayerViewPoint(TraceStart, CameraRotation);
//
//     const FVector TraceDirection = CameraRotation.Vector();
//     const FVector TraceEnd = TraceStart + (TraceDirection * InteractTraceLenght);
//
//     TArray<AActor*> ActorsToIgnore;
//     ActorsToIgnore.Add(GetOwner());
//
//     FHitResult ResultHit;
//
//     const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
//         GetWorld(),
//         TraceStart,
//         TraceEnd,
//         InteractTargetRadius,
//         ETraceTypeQuery::TraceTypeQuery1, //visibility trace
//         false,
//         ActorsToIgnore,
//         EDrawDebugTrace::None,
//         ResultHit,
//         true
//     );
//
//
//     if (bHit && GetInventoryComponent())
//     {
//         if (AItem* HitSword = Cast<AItem>(ResultHit.GetActor()))
//         {
//             if (GetCharacterStateComponent()->GetCurrentCharacterState().Form != ECharacterForm::ECF_Spectral)
//             {
//                 if (GetInventoryComponent()->TryAddItem(HitSword))
//                 {
//                     ActorsToIgnore.Add(HitSword);
//                     //HitSword->OnWallHit.AddDynamic(this, &AEntity::OnWallCollision);
//                 }
//             }
//         }
//         else if (ISpectralInteractable* SpectralObjectInteractable = Cast<ISpectralInteractable>(ResultHit.GetActor()))
//         {
//             if (GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
//             {
//                 SpectralObjectInteractable->Execute_SpectralInteract(ResultHit.GetActor(), this);
//             }
//         }
//         // else if (AItem* HitItem = Cast<AItem>(ResultHit.GetActor()))
//         // {
//         // 	HitItem->Use(this);
//         // 	Equipping(false);
//         // }
//     }
// }