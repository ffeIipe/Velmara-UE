#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HitInterface.generated.h"

UINTERFACE(MinimalAPI)
class UHitInterface : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API IHitInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void GetHit(const FVector& ImpactPoint);
	
	UFUNCTION(BlueprintNativeEvent)
	void GetFinished();
	
	UFUNCTION(BlueprintNativeEvent)
	bool CanBeFinished();

	UFUNCTION(BlueprintNativeEvent)
	bool IsLaunchable();

	UFUNCTION(BlueprintNativeEvent)
	void LaunchUp();
	
	UFUNCTION(BlueprintNativeEvent)
	void ShieldHit();
};
