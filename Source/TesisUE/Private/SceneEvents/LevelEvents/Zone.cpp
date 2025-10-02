// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/LevelEvents/Zone.h"

#include "SceneEvents/EnemySpawner.h"
#include "SceneEvents/EnemySpawner/SpawnPoint.h"
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
		Trigger->OnPlayerBeginOverlap.AddDynamic(EnemySpawner, &AEnemySpawner::StartSpawning);
	}

	for (ALever* Lever : Levers)
	{
		Lever->OnLeverActivation.BindUFunction(this, FName("OnLeverActivation"));
	}

	
}

void AZone::OnLeverActivation()
{
	if (CheckLevers())
	{
		EnemySpawner->EndSpawning();
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
