// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

// Forward declarations
class UInventory; // El widget de UI
class ASword; // Clase base para items/armas
class APlayerController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ShowInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void HideInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Logic")
	bool AddItem(ASword* ItemToAdd);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Logic")
	void SwitchItem(int ItemIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Logic")
	void DropItem(int ItemIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	FORCEINLINE UInventory* GetInventoryWidget() const { return InventoryWidget; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Logic")
	FORCEINLINE ASword* GetEquippedSword() const { return EquippedItem; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Logic")
	const TArray<ASword*>& GetInventoryItems() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State")
	bool bIsInventoryOpen = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State")
	int CurrentItemIndex = -1;

	/** Llama a esta función para actualizar visualmente el widget del inventario. Implementar en BP o C++. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void UpdateInventoryUI();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Config")
	int MaxInventorySize = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|UI")
	TSubclassOf<UInventory> InventoryWidgetClass;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State", meta = (AllowPrivateAccess = "true"))
	ASword* EquippedItem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State", meta = (AllowPrivateAccess = "true"))
	TArray<ASword*> InventoryItems;

	UPROPERTY()
	UInventory* InventoryWidget = nullptr;


	UPROPERTY()
	APlayerController* PlayerControllerRef = nullptr;

	UFUNCTION()
	void HandleSlotClicked(int ClickedIndex);
};