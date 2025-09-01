#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class IWeaponInterface;
struct FInventoryData;
class AItem;
class UInventory;
class APlayerController;
class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    int32 EquippedSlotIndex = -1;

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    TScriptInterface<IWeaponInterface> EquippedWeapon;

protected:
    virtual void BeginPlay() override;

    void InitializeValues(const FInventoryData& InventoryData);
    
    UPROPERTY(EditDefaultsOnly, Category = "Inventory UI")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY(Transient)
    UInventory* InventoryWidgetInstance = nullptr;

    UPROPERTY(Transient)
    APlayerController* PlayerControllerRef = nullptr;

    bool bIsInventoryOpen = false;

public:
    UPROPERTY(EditAnywhere, Category = "Inventory", Transient) //transient if it isn't needed to be saved 
    TArray<AItem*> InventorySlots;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    TScriptInterface<IWeaponInterface> GetWeaponEquipped() const { return EquippedWeapon; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TryAddWeapon(AItem* ItemToAdd);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void EquipWeaponFromSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropWeaponFromSlot(int32 SlotIndex);
    
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ShowInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void HideInventory();

    UFUNCTION(BlueprintPure, Category = "Inventory")
    bool IsInventoryOpen() const { return bIsInventoryOpen; }

    UFUNCTION(BlueprintPure, Category = "Inventory")
    const TArray<AItem*>& GetInventoryItems() const { return InventorySlots; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ChangeWeapon(int32 SlotIndex);

    UPROPERTY(EditDefaultsOnly, Category = "Inventory | Inputs")
    UInputAction* Slot1_InventoryAction;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory | Inputs")
    UInputAction* Slot2_InventoryAction;

    void UnequipCurrentWeapon();
    
    void UpdateInventoryUI();
    
    void Interact();

private:
    void InitializeInventoryWidget();

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    FName HandSocketName = FName("RightHandSocket");
    
    // --- Stats Assigned By Data Asset ---
    int32 MaxSlots = 2;

    AController* OwnerController = nullptr;
};