#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

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

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 MaxSlots = 2;

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    int32 EquippedSlotIndex = -1;


    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI", meta = (DisplayName = "RefreshInventoryUI"))
    void K2_RefreshInventoryUI(const TArray<AItem*>& Items);

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    AItem* EquippedItem = nullptr;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory UI")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY(Transient)
    UInventory* InventoryWidgetInstance = nullptr;

    UPROPERTY(Transient)
    APlayerController* PlayerControllerRef = nullptr;

    bool bIsInventoryOpen = false;


    //UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI", meta = (DisplayName = "RefreshInventoryUI"))
    //void K2_RefreshInventoryUI2(const TArray<AItem*>& Items);

public:
    UPROPERTY(EditAnywhere, Category = "Inventory", Transient) // Transient si no necesita guardarse
    TArray<AItem*> InventorySlots;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    AItem* GetEquippedItem() const { return EquippedItem; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TryAddItem(AItem* ItemToAdd);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void EquipItemFromSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItemFromSlot(int32 SlotIndex);

    // Funciones nuevas
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

    void UnequipCurrentItem();
    
    void UpdateInventoryUI();

private:
    void InitializeInventoryWidget();

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    FName HandSocketName = FName("RightHandSocket");


};