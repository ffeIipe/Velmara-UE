#include "SpectralMode/Lever.h"

#include "Kismet/GameplayStatics.h"
#include "SceneEvents/LevelEvents/Door.h"

void ALever::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);
	
	bLeverWasActivated = true;
	
	OnLeverActivation_Internal.ExecuteIfBound();

	OnLeverActivation();

	if (LeverTimer != 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(LeverTimerHandle, this, &ALever::CloseDoorsByTimer, LeverTimer, false);
	}
	
	for (ADoor* Door : DoorArray)
	{
		if (!IsValid(Door)) return;
		
		Door->OpenDoor();
		
		if (LeverActivatedSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), LeverActivatedSFX, GetActorLocation());
		}
	}
}

void ALever::BeginPlay()
{
	Super::BeginPlay();

	AddToSpectralObjects();
}

void ALever::CloseDoorsByTimer()
{
	ClearDoorTimer();

	bLeverWasActivated = false;
	
	OnLeverDeactivation_Internal.ExecuteIfBound();

	OnLeverDeactivation();
	
	for (ADoor* Door : DoorArray)
	{
		if (!IsValid(Door)) return;
		
		Door->CloseDoor();
		
		if (LeverDeactivatedSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), LeverDeactivatedSFX, GetActorLocation());
		}
	}
}

void ALever::ClearDoorTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(LeverTimerHandle);
}