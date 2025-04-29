#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

<<<<<<< Updated upstream
class UInventory;
class ASword;
=======
class AItem;
class UUserWidget;
>>>>>>> Stashed changes
class APlayerController;
class UInputAction;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

<<<<<<< Updated upstream
public:	
	UInventoryComponent();

	UFUNCTION(BlueprintCallable)
	void ShowInventory();

	UFUNCTION(BlueprintCallable)
	void HideInventory();

	UFUNCTION()
	void SwitchWeapon(int NewWeaponIndex);

	FORCEINLINE UInventory* GetInventoryWidget() { return InventoryWidget; };

	bool bIsInventoryOpen;

	int CurrentIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess))
	ASword* EquippedWeapon;

	UPROPERTY(VisibleAnywhere)
	TArray<ASword*> InventoryWeapons;

=======
public:
    UInventoryComponent();
>>>>>>> Stashed changes

protected:
    virtual void BeginPlay() override;

<<<<<<< Updated upstream
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UInventory> InventoryWidgetClass;

private:
	UPROPERTY(EditDefaultsOnly)
	UInventory* InventoryWidget;

	APlayerController* PlayerController = nullptr;
=======
    UPROPERTY(EditDefaultsOnly, Category = "Inventory UI")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    int32 MaxSlots = 2;

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient) // Transient si no necesita guardarse
        TArray<AItem*> InventorySlots;

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    AItem* EquippedItem = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Inventory", Transient)
    int32 EquippedSlotIndex = -1;

    UPROPERTY(Transient)
    UUserWidget* InventoryWidgetInstance = nullptr;

    UPROPERTY(Transient)
    APlayerController* PlayerControllerRef = nullptr;

    bool bIsInventoryOpen = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI", meta = (DisplayName = "RefreshInventoryUI"))
    void K2_RefreshInventoryUI(const TArray<AItem*>& Items);

public:
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

private:
    void InitializeInventoryWidget();
    void UpdateInventoryUI();
    void UnequipCurrentItem();

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    FName HandSocketName = FName("RightHandSocket");
>>>>>>> Stashed changes
};