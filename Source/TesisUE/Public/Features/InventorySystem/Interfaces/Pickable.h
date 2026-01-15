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
	virtual void Pick(AActor* NewOwner) = 0;

	virtual void OnEquip(AActor* User) = 0;
	
	virtual void OnUnequip(AActor* User) = 0;

	virtual bool ShouldConsumeOnUse() const { return false; }
};
