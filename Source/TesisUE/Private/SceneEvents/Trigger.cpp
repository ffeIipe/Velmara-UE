// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/Trigger.h"
#include "Components/BoxComponent.h"
#include "Player/PlayerMain.h"

// Sets default values
ATrigger::ATrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void ATrigger::BeginPlay()
{
	Super::BeginPlay();

	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &ATrigger::OnSphereBeginOverlap);
	BoxCollider->OnComponentEndOverlap.AddDynamic(this, &ATrigger::OnSphereEndOverlap);
}


void ATrigger::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Player = Cast<APlayerMain>(OtherActor);
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Player)
	{
		Player->SetOverlappingItem(nullptr);
	}
}

void ATrigger::DisableCollision()
{
	if (BoxCollider)
	{
		BoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

