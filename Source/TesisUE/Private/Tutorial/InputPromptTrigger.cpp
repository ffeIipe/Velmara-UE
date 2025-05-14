// Fill out your copyright notice in the Description page of Project Settings.


#include "Tutorial/InputPromptTrigger.h"
#include "Components/BoxComponent.h"
#include "Blueprint/UserWidget.h"
#include "HUD/PlayerMainHUD.h"
#include "Tutorial/PromptWidgetComponent.h"

AInputPromptTrigger::AInputPromptTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    BoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollider->SetCollisionObjectType(ECC_WorldDynamic);
    BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
    BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    PromptWidgetComponent = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptWidgetComponent"));
    PromptWidgetComponent->SetupAttachment(GetRootComponent());
    PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    PromptWidgetComponent->SetDrawAtDesiredSize(true);
}

void AInputPromptTrigger::BeginPlay()
{
    Super::BeginPlay();
    
    if (PromptWidgetComponent)
    {
        PromptWidgetComponent->EnablePromptWidget(false);
    }
}

void AInputPromptTrigger::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

    if (Player && PromptWidgetComponent)
    {
        PromptWidgetComponent->EnablePromptWidget(true);
    }
}

void AInputPromptTrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

    Player = Cast<APlayerMain>(OtherActor);

    if (Player)
    {
        if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
        {
            PromptWidgetComponent->EnablePromptWidget(false);
        }
    }
}