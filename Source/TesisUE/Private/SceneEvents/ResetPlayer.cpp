// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/ResetPlayer.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "GameFramework/GameModeBase.h"
#include <Player/PlayerMain.h>
#include "GameFramework/PlayerStart.h"
#include "Components/MementoComponent.h"

AResetPlayer::AResetPlayer()
{
    BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
    BoxCollider->SetupAttachment(GetRootComponent());
}

void AResetPlayer::BeginPlay()
{
    Super::BeginPlay();

    BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AResetPlayer::OnBoxOverlap);
    PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())); 
}

void AResetPlayer::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController) return;

    APawn* ControlledPawn = PlayerController->GetPawn();

    APlayerMain* Player = Cast<APlayerMain>(OtherActor);

    if (OtherActor == ControlledPawn)
    {
        FTransform Transform;

        if (Player)
        {
            ControlledPawn->SetActorTransform(Player->MementoComponent->GetLastTransform());
        }
        else
        {
            ControlledPawn->SetActorTransform(PlayerStart->GetActorTransform());
        }
    }
}