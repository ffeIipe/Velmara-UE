// Fill out your copyright notice in the Description page of Project Settings.


#include "Tutorial/InputPromptTrigger.h"
#include "Components/BoxComponent.h"
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
    PromptWidgetComponent->SetPromptRowName("Interact");

    OnEntityBeginOverlap.AddDynamic(PromptWidgetComponent, &UPromptWidgetComponent::EnablePromptWidget);
    OnEntityEndOverlap.AddDynamic(PromptWidgetComponent, &UPromptWidgetComponent::DisablePromptWidget);
}

void AInputPromptTrigger::BeginPlay()
{
    Super::BeginPlay();
    
    if (PromptWidgetComponent)
    {
        PromptWidgetComponent->DisablePromptWidget();
    }
}