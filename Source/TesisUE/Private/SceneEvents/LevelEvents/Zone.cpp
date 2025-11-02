// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/LevelEvents/Zone.h"

#include "SceneEvents/EnemySpawner.h"
#include "SpectralMode/Lever.h"


AZone::AZone()
{
	PrimaryActorTick.bCanEverTick = false;
	
}

void AZone::BeginPlay()
{
	Super::BeginPlay();

	if (Trigger)
	{
		Trigger->OnPlayerControllerBeginOverlap.AddDynamic(EnemySpawner, &AEnemySpawner::StartSpawning);
	}

	for (ALever* Lever : Levers)
	{
		Lever->OnLeverActivation_Internal.BindUFunction(this, FName("OnLeverActivation"));
	}
}

void AZone::OnLeverActivation()
{
	if (CheckLevers())
	{
		EnemySpawner->EndSpawning();
		Trigger->OnPlayerControllerBeginOverlap.Clear();
		for (ALever* Lever : Levers)
		{
			Lever->ClearDoorTimer();
		}
	}
}

bool AZone::CheckLevers()
{
	for (const ALever* Lever : Levers)
	{
		if (!Lever->bLeverWasActivated)
		{
			return false;	
		}
	}
	return true;
}
