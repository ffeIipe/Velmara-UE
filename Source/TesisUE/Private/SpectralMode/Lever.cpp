#include "SpectralMode/Lever.h"
#include "Kismet/GameplayStatics.h"

void ALever::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);

	for (AActor* Door : DoorArray)
	{
		if (!IsValid(Door)) return;
		
		OpenDoors(Door);

		if (OpenDoorSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenDoorSFX, Door->GetActorLocation());
		}
	}
}
