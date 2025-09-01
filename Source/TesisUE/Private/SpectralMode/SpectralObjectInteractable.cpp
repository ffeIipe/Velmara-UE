#include "SpectralMode/SpectralObjectInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SpectralObjectComponent.h"

ASpectralObjectInteractable::ASpectralObjectInteractable()
{
	SpectralObjectComponent->VisibleTo = ECharacterMode::ECM_Spectral;
}

void ASpectralObjectInteractable::Use(ACharacter* Character)
{
	Super::Use(Character);

	for (AActor* Door : DoorArray)
	{
		if (!IsValid(Door)) return;

		else
		{
			OpenDoors(Door);

			if (OpenDoorSFX)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenDoorSFX, Door->GetActorLocation());
			}
		}
	}
}

void ASpectralObjectInteractable::SpectralInteract_Implementation(ACharacter* Character)
{
	Use(Character);
}
