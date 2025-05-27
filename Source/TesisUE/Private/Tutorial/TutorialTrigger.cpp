// Fill out your copyright notice in the Description page of Project Settings.

#include "Tutorial/TutorialTrigger.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/TutorialWidget.h"
#include "GameFramework/Character.h"

ATutorialTrigger::ATutorialTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    RootComponent = TriggerVolume;
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATutorialTrigger::BeginPlay()
{
    Super::BeginPlay();

    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATutorialTrigger::OnPlayerEnter);
    bHasBeenActivated = false;
}

void ATutorialTrigger::OnPlayerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                     bool bFromSweep, const FHitResult& SweepResult)
{
    if (bHasBeenActivated || !OtherActor->IsA(ACharacter::StaticClass()))
        return;

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetPause(true);
        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;

        if (TutorialWidgetClass)
        {
            UTutorialWidget* Widget = CreateWidget<UTutorialWidget>(PC, TutorialWidgetClass);
            if (Widget)
            {
                Widget->InitializeTutorial(TutorialVideo, TutorialTitleText, TutorialParragraphText, this);
                Widget->AddToViewport();
                Widget->PlayFadeIn();
            }
        }
        bHasBeenActivated = true;
    }
}

