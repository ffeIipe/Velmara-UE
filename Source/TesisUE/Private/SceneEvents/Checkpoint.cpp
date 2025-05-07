// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/Checkpoint.h"
#include "Player/PlayerMain.h"
#include "Components/MementoComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameStateBase.h"
#include "SceneEvents/NewGameInstance.h"

void ACheckpoint::OnSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    APlayerMain* OverlappingPlayer = Cast<APlayerMain>(OtherActor);

    if (OverlappingPlayer)
    {
        DisableCollision();

        UNewGameInstance* GameInst = Cast<UNewGameInstance>(GetGameInstance());
        if (GameInst)
        {
            if (GameInst->SavePlayerProgress(GameInst->ActiveSaveSlotIndex))
            {
                UE_LOG(LogTemp, Log, TEXT("ACheckpoint: Player progress saved via GameInstance. Slot: %d"), GameInst->ActiveSaveSlotIndex);
            }
        }

        SetLifeSpan(1.f);
    }
}