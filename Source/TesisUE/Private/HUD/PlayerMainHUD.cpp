// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PlayerMainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/InputPromptWidget.h"
#include "Kismet/GameplayStatics.h"
#include "HUD/PlayerMainWidget.h"
#include "HUD/PaladinBossHealthBar.h"

void APlayerMainHUD::BeginPlay()
{
    Super::BeginPlay();

    PlayerMainWidgetInstance = CreateWidget<UPlayerMainWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerMainWidgetClass);

    if (PlayerMainWidgetInstance)
    {
        PlayerMainWidgetInstance->AddToViewport();
    }
}

void APlayerMainHUD::SetPaladinBossHealthBar()
{
    if (PaladinBossMainWidgetClass && !bAlreadySetted)
    {
        bAlreadySetted = true;

        PaladinBossHealthBarInstance = CreateWidget<UPaladinBossHealthBar>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PaladinBossMainWidgetClass);
        PaladinBossHealthBarInstance->AddToViewport();
    }
    else return;
}

void APlayerMainHUD::RemovePaladinBossHealthBar()
{
    if (PaladinBossHealthBarInstance && PaladinBossHealthBarInstance->IsInViewport())
    {
        PaladinBossHealthBarInstance->RemoveFromParent();
    }
}

void APlayerMainHUD::TogglePaladinUI(bool Bool)
{
    if (Bool)
    {
        if (!PaladinUIInstance)
        {
            PaladinUIInstance = CreateWidget<UUserWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PaladinUIWidgetClass);
            PaladinUIInstance->AddToViewport();
        }
        else
        {
            PaladinUIInstance->AddToViewport();
        }
    }
    else
    {
        if (PaladinUIInstance)
        {
            PaladinUIInstance->RemoveFromParent();
            PaladinUIInstance = nullptr;
        }
    }
}

void APlayerMainHUD::TogglePlayerUI(bool Bool)
{
    if (Bool)
    {
        if (!PlayerMainWidgetInstance)
        {
            PlayerMainWidgetInstance = CreateWidget<UPlayerMainWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerMainWidgetClass);
            PlayerMainWidgetInstance->AddToViewport();
        }
        else
        {
            PlayerMainWidgetInstance->AddToViewport();
        }
    }
    else
    {
        if (PlayerMainWidgetInstance)
        {
            PlayerMainWidgetInstance->RemoveFromParent();
            PlayerMainWidgetInstance = nullptr;
        }
    }
}
