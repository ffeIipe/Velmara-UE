#pragma once

#include "CoreMinimal.h"
#include "SpectralObject.h"
#include "Lever.generated.h"

class ADoor;

UCLASS()
class TESISUE_API ALever : public ASpectralObject
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_DELEGATE(FOnLeverActivation_Internal);
	DECLARE_DYNAMIC_DELEGATE(FOnLeverDeactivation_Internal);

public:
	FOnLeverActivation_Internal OnLeverActivation_Internal;
	FOnLeverDeactivation_Internal OnLeverDeactivation_Internal;

	bool bLeverWasActivated = false;
	
	void ClearDoorTimer();
	
protected:
	virtual void OnEnteredInventory_Implementation(AActor* NewOwner) override;

	virtual void BeginPlay() override;

	void CloseDoorsByTimer();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLeverActivation();

	UFUNCTION(BlueprintImplementableEvent)
	void OnLeverDeactivation();

	UPROPERTY(EditAnywhere, Category = "Properties")
	TArray<ADoor*> DoorArray;
	
	UPROPERTY(EditAnywhere, Category = "Properties")
	USoundBase* LeverActivatedSFX;

	UPROPERTY(EditAnywhere, Category = "Properties")
	USoundBase* LeverDeactivatedSFX;

	UPROPERTY(EditAnywhere, Category = "Properties")
	float LeverTimer = 30.f;

	FTimerHandle LeverTimerHandle;
};
