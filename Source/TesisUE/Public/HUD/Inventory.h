// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory.generated.h"



UCLASS()
class TESISUE_API UInventory : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponButtonClicked, int, WeaponIndex);

	UPROPERTY(BlueprintAssignable)
	FOnWeaponButtonClicked OnWeaponButtonClickedEvent;

private:
	UFUNCTION(BlueprintCallable, meta = (AllowPrivateAccess = "true"))
	void OnClicked(int NewWeaponIndex);

	int WeaponIndex;
};
