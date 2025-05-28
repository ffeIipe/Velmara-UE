// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/ResetPlayer.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "Player/PlayerMain.h"
#include "GameFramework/PlayerStart.h"
#include "Components/MementoComponent.h"
#include "Interfaces/MementoEntity.h"

void AResetPlayer::BeginPlay()
{
    Super::BeginPlay();

    PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())); 
}

void AResetPlayer::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (APawn* OverlappingActor = Cast<APawn>(OtherActor)) // cuando ponés AActor acá como filtro explota, por favor dejar en APawn :)
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

        if (IMementoEntity* MementoEntity = Cast<IMementoEntity>(PlayerController->GetPawn()))
        {
            if (UMementoComponent* MementoComp = MementoEntity->Execute_GetMementoComponent(OtherActor))
            {
                FTransform LastTransform = MementoComp->GetLastSavedTransform();
                OverlappingActor->SetActorTransform(LastTransform);
            }
        }
    }
}

void AResetPlayer::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    
}