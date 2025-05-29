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
    //Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (APawn* OverlappingPawn = Cast<APawn>(OtherActor))
    {
        AController* TempController = OverlappingPawn->GetController();

        if (TempController == UGameplayStatics::GetPlayerController(this, 0))
        {
            DisableCollision();

            UNewGameInstance* GameInst = Cast<UNewGameInstance>(GetGameInstance());
            if (GameInst)
            {
                if (GameInst->SavePlayerProgress(GameInst->ActiveSaveSlotIndex))
                {
                    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Emerald, FString("Player progress saved via GameInstance."));
                    UE_LOG(LogTemp, Log, TEXT("ACheckpoint: Player progress saved via GameInstance. Slot: %d"), GameInst->ActiveSaveSlotIndex);
                }
            }

            Destroy();
        }
        else
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString("Not controller found. Failed to save." + OverlappingPawn->GetController()->GetName()));
        }
    }
}