#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OrbPoolingInterface.generated.h"

UINTERFACE(MinimalAPI)
class UOrbPoolingInterface : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API IOrbPoolingInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void ActivateOrb(const FVector& Location, const FRotator& Rotation);
	
	UFUNCTION(BlueprintNativeEvent)
	void DeactivateOrb();
};
