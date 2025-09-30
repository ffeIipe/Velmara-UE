#pragma once

#include "CoreMinimal.h"
#include "SpectralObject.h"
#include "Lever.generated.h"

UCLASS()
class TESISUE_API ALever : public ASpectralObject
{
	GENERATED_BODY()

protected:
	virtual void Pick(AActor* NewOwner) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OpenDoors(AActor* DoorToOpen);
	
	UPROPERTY(EditAnywhere)
	TArray<AActor*> DoorArray;

	UPROPERTY(EditAnywhere)
	USoundBase* OpenDoorSFX;

};
