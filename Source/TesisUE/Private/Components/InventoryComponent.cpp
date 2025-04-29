#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    InventorySlots.Init(nullptr, MaxSlots);

    PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    InitializeInventoryWidget();
}

void UInventoryComponent::ChangeWeapon(int32 SlotIndex)
{
    if (InventorySlots.IsValidIndex(SlotIndex))
    {
        EquipItemFromSlot(SlotIndex);
    }
}

void UInventoryComponent::InitializeInventoryWidget()
{
    if (InventoryWidgetClass && PlayerControllerRef && PlayerControllerRef->IsLocalController() && !InventoryWidgetInstance)
    {
        InventoryWidgetInstance = CreateWidget<UUserWidget>(PlayerControllerRef, InventoryWidgetClass);
        if (InventoryWidgetInstance)
        {
            InventoryWidgetInstance->AddToViewport();
            InventoryWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

bool UInventoryComponent::TryAddItem(AItem* ItemToAdd)
{
    if (!ItemToAdd) return false;

    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i] == nullptr)
        {
            InventorySlots[i] = ItemToAdd;
            ItemToAdd->EnableVisuals(false);
            UpdateInventoryUI();
            return true;
        }
    }
    return false;
}

void UInventoryComponent::EquipItemFromSlot(int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr)
    {
        return;
    }

    //if (SlotIndex == EquippedSlotIndex)
    //{
    //    // UnequipCurrentItem();
    //    // HideInventory(); // Cierra inventario si desequipas
    //    return;
    //}

    UnequipCurrentItem();

    AItem* ItemToEquip = InventorySlots[SlotIndex];
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

    if (OwnerCharacter && ItemToEquip)
    {
        USceneComponent* AttachParent = OwnerCharacter->GetMesh();
        if (AttachParent)
        {
            ItemToEquip->Equip(AttachParent, HandSocketName, OwnerCharacter, OwnerCharacter);
            ItemToEquip->EnableVisuals(true);

            EquippedItem = ItemToEquip;
            EquippedSlotIndex = SlotIndex;
        }
    }

    UpdateInventoryUI();
    HideInventory();
}

void UInventoryComponent::DropItemFromSlot(int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr)
    {
        return;
    }

    AItem* ItemToDrop = InventorySlots[SlotIndex];

    if (ItemToDrop == EquippedItem)
    {
        UnequipCurrentItem();
    }

    InventorySlots[SlotIndex] = nullptr;

    AActor* Owner = GetOwner();
    if (Owner && GetWorld() && ItemToDrop)
    {
        ItemToDrop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        ItemToDrop->EnableVisuals(true);

        FVector DropLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.0f + FVector(0, 0, 50.0f);
        FRotator DropRotation = Owner->GetActorRotation();

        ItemToDrop->TeleportTo(DropLocation, DropRotation, false, true);
        // ItemToDrop->ItemState = EItemState::EIS_Hovering;
    }

    UpdateInventoryUI();
}

void UInventoryComponent::UnequipCurrentItem()
{
    if (EquippedItem)
    {
        EquippedItem->EnableVisuals(false);
        EquippedItem = nullptr;
        EquippedSlotIndex = -1;
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
        K2_RefreshInventoryUI(InventorySlots);
    }
}