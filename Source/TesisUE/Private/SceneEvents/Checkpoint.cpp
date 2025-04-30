// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/Checkpoint.h"
#include "Player/PlayerMain.h"
#include "Components/MementoComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameStateBase.h"

void ACheckpoint::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	DisableCollision();

	if (Player)
	{
		AGameStateBase* BaseGameState = UGameplayStatics::GetGameState(GetWorld()); //father
		if (ANewGameStateBase* NewGameState = Cast<ANewGameStateBase>(BaseGameState)) //child
		{
			NewGameState->SaveAllMementoStates();
			SetLifeSpan(1.f);
		}
	}
}
