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
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

    if (PlayerController)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Black, FString("VALID CONTROLLER"));
        APawn* ControlledPawn = PlayerController->GetPawn();

        if (OtherActor == ControlledPawn)
        {
            
            FTransform LastTransform = ControlledPawn->GetComponentByClass<UMementoComponent>()->GetLastSavedTransform();

            ControlledPawn->SetActorTransform(LastTransform);
        }
    }
}