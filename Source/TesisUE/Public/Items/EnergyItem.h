#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "EnergyItem.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API AEnergyItem : public AItem
{
	GENERATED_BODY()
	
public:
	void Use(class ACharacter* TargetCharacter) override;

private:
	bool bWasUsed = false;

	UPROPERTY(EditAnywhere, Category = "Stats | Health")
	float HealthToIncrease;
	
	UPROPERTY(EditAnywhere, Category = "Stats | Energy")
	float EnergyToIncrease;
	
	UPROPERTY(EditAnywhere, Category = "Stats | FX")
	USoundBase* OpenSFX;

	UPROPERTY(EditAnywhere, Category = "Stats | FX")
	USoundBase* HealthSFX;

	UPROPERTY(EditAnywhere, Category = "Stats | FX")
	USoundBase* EnergySFX;
};
