#pragma once

#include "CoreMinimal.h"
#include "SpectralMode/SpectralObject.h"
#include "Interfaces/SpectralInteractable.h"
#include "SpectralObjectInteractable.generated.h"

UCLASS()
class TESISUE_API ASpectralObjectInteractable : public ASpectralObject, public ISpectralInteractable
{
	GENERATED_BODY()

public:
	ASpectralObjectInteractable();

	void Use(ACharacter* Character) override;

	void SpectralInteract_Implementation(ACharacter* Character) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OpenDoors(AActor* DoorToOpen);

	UPROPERTY(EditAnywhere)
	TArray<AActor*> DoorArray;

	UPROPERTY(EditAnywhere)
	USoundBase* OpenDoorSFX;

};
