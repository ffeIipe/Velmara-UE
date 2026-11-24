#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Inventory.generated.h"

class AItem;
class IPickable;
class UInventoryComponent;

UCLASS()
class TESISUE_API UInventory : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void RefreshInventoryUI(const TArray<TScriptInterface<IPickable>>& Items);

	virtual void RefreshInventoryUI_Implementation(const TArray<TScriptInterface<IPickable>>& Items) {}

	virtual void BindInventoryComponent(UInventoryComponent* InInventoryComponent) {}
};
