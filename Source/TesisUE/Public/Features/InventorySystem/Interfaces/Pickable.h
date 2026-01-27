#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Pickable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPickable : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API IPickable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnEnteredInventory(AActor* NewOwner);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnRemovedFromInventory();

	virtual bool ShouldConsumeOnUse() const { return false; }

	//virtual FItemData GetItemData() const = 0;
};
