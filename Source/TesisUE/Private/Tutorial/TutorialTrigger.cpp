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

    BoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATutorialTrigger::BeginPlay()
{
    Super::BeginPlay();

    bHasBeenActivated = false;

    OnPlayerBeginOverlap.AddDynamic(this, &ATutorialTrigger::EnableTutorial);
}

//void ATutorialTrigger::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
//{
//    Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
//
//    EnableTutorial();
//}

void ATutorialTrigger::EnableTutorial()
{
    if (Player && !bHasBeenActivated)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

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

