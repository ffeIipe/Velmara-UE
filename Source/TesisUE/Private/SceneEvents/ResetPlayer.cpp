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
    if (APawn* OverlappingActor = Cast<APawn>(OtherActor))
    {
        //APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

        //if (OverlappingActor->GetController() == PlayerController)

        if (UMementoComponent* MementoComp = OverlappingActor->GetComponentByClass<UMementoComponent>())
        {
            FTransform LastTransform = MementoComp->GetLastSavedTransform();
            OverlappingActor->SetActorTransform(LastTransform);
        }
    }
}