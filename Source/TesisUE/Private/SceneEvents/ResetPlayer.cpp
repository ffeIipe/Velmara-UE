// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/ResetPlayer.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "GameFramework/GameModeBase.h"
#include <Player/PlayerMain.h>
#include "GameFramework/PlayerStart.h"

AResetPlayer::AResetPlayer()
{
    BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
    BoxCollider->SetupAttachment(GetRootComponent());
}

void AResetPlayer::BeginPlay()
{
    Super::BeginPlay();

    BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AResetPlayer::OnBoxOverlap);
}

void AResetPlayer::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{    
    APlayerMain* Player = Cast<APlayerMain>(OtherActor);
    if (OtherActor && Player)
    {
        APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (PlayerController)
        {
            APlayerStart* PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
            PlayerController->GetPawn()->SetActorLocation(PlayerStart->GetActorLocation());
            PlayerController->GetPawn()->SetActorRotation(PlayerStart->GetActorRotation());
        }
    }
}

