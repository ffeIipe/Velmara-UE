#include "SpectralMode/Lever.h"

#include "Kismet/GameplayStatics.h"

void ALever::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, "Interact");
	
	OnLeverActivation.ExecuteIfBound();
	bLeverWasActivated = true;
	
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

void ALever::BeginPlay()
{
	Super::BeginPlay();

	AddToSpectralObjects();
}
