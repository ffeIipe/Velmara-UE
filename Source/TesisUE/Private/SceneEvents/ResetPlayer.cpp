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

void AResetPlayer::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    // Obtener el PlayerController
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController) return;

    // Verificar si el OtherActor es el Pawn controlado actualmente
    APawn* ControlledPawn = PlayerController->GetPawn();
    if (OtherActor == ControlledPawn)
    {
        // Obtener el PlayerStart
        APlayerStart* PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
        if (PlayerStart)
        {
            ControlledPawn->SetActorLocation(PlayerStart->GetActorLocation());
            ControlledPawn->SetActorRotation(PlayerStart->GetActorRotation());
        }
    }
}

