#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LaunchableInterface.generated.h"

UINTERFACE(MinimalAPI)
class ULaunchableInterface : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API ILaunchableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	bool IsLaunchable();

	UFUNCTION(BlueprintNativeEvent)
	void LaunchUp();
	
	UFUNCTION(BlueprintNativeEvent)
	AActor* GetInterfaceOwner();
};
