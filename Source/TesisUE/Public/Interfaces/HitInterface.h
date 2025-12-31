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
	virtual void GetHit(AActor* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived) = 0;
	
	virtual bool IsHittable() = 0;
};