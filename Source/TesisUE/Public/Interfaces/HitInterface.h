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
	void LaunchUp(const FVector& InstigatorLocation);
	
	UFUNCTION(BlueprintNativeEvent)
	bool IsLaunchable();


};
