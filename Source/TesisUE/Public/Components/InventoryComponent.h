#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class IAnimatorProvider;
class ICharacterStateProvider;
class IControllerProvider;
class IWeaponInterface;
struct FInventoryData;
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
    TScriptInterface<IWeaponInterface> CurrentWeapon;

    void InitializeValues(const FInventoryData& InventoryData);
protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(EditDefaultsOnly, Category = "Inventory UI")
    TSubclassOf<UUserWidget> InventoryWidgetClass;

    UPROPERTY(Transient)
    UInventory* InventoryWidgetInstance = nullptr;

    UPROPERTY(Transient)
    APlayerController* PlayerControllerRef = nullptr;

    bool bIsInventoryOpen = false;

public:
    UPROPERTY(EditAnywhere, Category = "Inventory", Transient) //transient if it isn't needed to be saved 
    TArray<TScriptInterface<IWeaponInterface>> InventorySlots;

    UFUNCTION(BlueprintPure, Category = "Inventory")
    TScriptInterface<IWeaponInterface> GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TryAddWeapon(TScriptInterface<IWeaponInterface> WeaponToAdd);

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
    const TArray<TScriptInterface<IWeaponInterface>>& GetInventoryItems() const { return InventorySlots; }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void ChangeWeapon(int32 SlotIndex);

    void ToggleInventorySlot();
    
    void UnequipCurrentWeapon();
    
    void UpdateInventoryUI();

    TScriptInterface<IWeaponInterface> PerformInteract();

private:
    void InitializeInventoryWidget();

    // UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    // FName HandSocketName = FName("RightHandSocket");
    
    // --- Stats Assigned By Data Asset ---
    int32 MaxSlots;
    float InteractTraceLenght;
    float InteractTargetRadius;
    
    UPROPERTY()
    AController* OwnerController = nullptr;
    TScriptInterface<IControllerProvider> ControllerProvider;
    TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
    TScriptInterface<IAnimatorProvider> AnimatorProvider;
};