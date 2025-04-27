// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UInventory;
class ASword;
class APlayerController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

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


protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UInventory> InventoryWidgetClass;

private:
	UPROPERTY(EditDefaultsOnly)
	UInventory* InventoryWidget;

	APlayerController* PlayerController = nullptr;
};