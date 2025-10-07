// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/LevelEvents/Door.h"


ADoor::ADoor()
{
	PrimaryActorTick.bCanEverTick = false;

	DoorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("DoorMesh");
	RootComponent = DoorMeshComponent;
}

void ADoor::BeginPlay()
{
	Super::BeginPlay();
	
}