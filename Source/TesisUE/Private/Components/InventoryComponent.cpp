#include "Components/InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/CharacterStateComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HUD/Inventory.h"

#include "DataAssets/EntityData.h"
#include "Interfaces/AnimatorProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/Pickable.h"
#include "Player/CharacterWeaponStates.h"
#include "SpectralMode/Interfaces/Spectral.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    OwnerController = PlayerControllerRef;
    ControllerProvider = GetOwner();
    CharacterStateProvider = GetOwner();
    AnimatorProvider = GetOwner();

    InitializeInventoryWidget();
}

void UInventoryComponent::InitializeValues(const FInventoryData& InventoryData)
{
    MaxSlots = InventoryData.MaxSlots;
    InteractTraceLenght = InventoryData.InteractTraceLenght;
    InteractTargetRadius = InventoryData.InteractTargetRadius;

    InventorySlots.Init(nullptr, MaxSlots);
}

void UInventoryComponent::ChangeWeapon(const int32 SlotIndex)
{
    if (InventorySlots.IsValidIndex(SlotIndex))
    {
        EquipWeaponFromSlot(SlotIndex);
    }
}

void UInventoryComponent::ToggleInventorySlot()
{
    EquippedSlotIndex++;
    if (EquippedSlotIndex >= MaxSlots) EquippedSlotIndex = 0;
    
    ChangeWeapon(EquippedSlotIndex);
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

bool UInventoryComponent::TryAddWeapon(const TScriptInterface<IWeaponInterface> WeaponToAdd)
{
    if (!WeaponToAdd) return false;

    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (InventorySlots[i] == nullptr)
        {
            InventorySlots[i] = WeaponToAdd;

            ChangeWeapon(i);

            UpdateInventoryUI();
            return true;
        }
    }

    return false; // Inventario lleno
}

void UInventoryComponent::EquipWeaponFromSlot(const int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr) return;
    
    //UnequipCurrentWeapon();
    
    if (CurrentWeapon)
    {
        CurrentWeapon->DisableVisuals();
    }
    
    if (const TScriptInterface<IPickable> Pickable = InventorySlots[SlotIndex].GetObject()) Pickable->Pick(GetOwner());

    if (InventorySlots.IsValidIndex(SlotIndex))
    {
        CurrentWeapon = InventorySlots[SlotIndex];
        EquippedSlotIndex = SlotIndex;
        CurrentWeapon->EnableVisuals();
    }
    
    //AnimatorProvider->ChangeWeaponAnimationState();
    //UpdateInventoryUI();
}

void UInventoryComponent::DropWeaponFromSlot(const int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex] == nullptr)
    {
        return;
    }

    InventorySlots[SlotIndex] = CurrentWeapon;
    
    if (CurrentWeapon)
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
    if (CurrentWeapon)
    {
        CurrentWeapon->EnableVisuals();
        CurrentWeapon = nullptr;
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
        const FInputModeGameOnly InputMode;
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

TScriptInterface<IWeaponInterface> UInventoryComponent::PerformInteract()
{
    if (IsInventoryOpen()) return nullptr;
    
    FVector TraceStart;
    FRotator CameraRotation;
    OwnerController->GetPlayerViewPoint(TraceStart, CameraRotation);
    const FVector TraceDirection = CameraRotation.Vector();
    const FVector TraceEnd = TraceStart + (TraceDirection * InteractTraceLenght);
    
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    FHitResult ResultHit;
    
    const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
        GetWorld(),
        GetOwner()->GetActorLocation() + FVector(0.f, 100.f, 0.f),
        TraceEnd,
        InteractTargetRadius,
        UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel5), //item trace
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForDuration,
        ResultHit,
        true
    );

    const bool bIsSpectral = CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->IsModeEqualToAny(
    {
        ECharacterModeStates::ECMS_Spectral 
    });
    
    if (bHit)
    {
        if (const TScriptInterface<IPickable> Pickable = ResultHit.GetActor())
        {
            if (const TScriptInterface<IWeaponInterface> WeaponReached = Pickable.GetObject(); TryAddWeapon(WeaponReached))
            {
                if (!bIsSpectral)
                {
                    ActorsToIgnore.Add(Cast<AActor>(WeaponReached.GetObject()));
                    return WeaponReached;
                }
            }
            else
            {
                Pickable->Pick(GetOwner());
            }
        }
    }
    return nullptr;
}