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

	void BeginPlay() override;

	void SpectralInteract_Implementation() override;

	UPROPERTY(EditAnywhere)
	TArray<UStaticMeshComponent*> DoorMeshArray;

	UPROPERTY(EditAnywhere)
	USoundBase* OpenDoorSFX;

};
