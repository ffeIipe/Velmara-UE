// Fill out your copyright notice in the Description page of Project Settings.


#include "Tutorial/InputPromptTrigger.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/InputPromptWidget.h"
#include "Blueprint/UserWidget.h"
#include "HUD/PlayerMainHUD.h"
#include "Tutorial/PromptWidgetComponent.h"

AInputPromptTrigger::AInputPromptTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerArea = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerArea"));
    TriggerArea->SetupAttachment(GetRootComponent());
    TriggerArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerArea->SetCollisionObjectType(ECC_WorldDynamic);
    TriggerArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    PromptWidgetComponent = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptWidgetComponent"));
    PromptWidgetComponent->SetupAttachment(GetRootComponent());
    PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    PromptWidgetComponent->SetDrawAtDesiredSize(true);
    PromptWidgetComponent->SetVisibility(false);
}

void AInputPromptTrigger::BeginPlay()
{
    Super::BeginPlay();

    TriggerArea->OnComponentBeginOverlap.AddDynamic(this, &AInputPromptTrigger::OnOverlapBegin);
    TriggerArea->OnComponentEndOverlap.AddDynamic(this, &AInputPromptTrigger::OnOverlapEnd);
}

void AInputPromptTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PromptWidgetComponent->SetVisibility(true);
    }
}

void AInputPromptTrigger::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {

        PromptWidgetComponent->SetVisibility(false);
    }
}