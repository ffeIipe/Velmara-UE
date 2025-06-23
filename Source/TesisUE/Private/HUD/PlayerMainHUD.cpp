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

    PlayerMainWidgetInstance = CreateWidget<UPlayerMainWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerMainWidget->GetClass());

    if (PlayerMainWidgetInstance)
    {
        PlayerMainWidgetInstance->AddToViewport();
    }
}

void APlayerMainHUD::SetPaladinBossHealthBar()
{
    if (PaladinBossMainWidget && !bAlreadySetted)
    {
        bAlreadySetted = true;

        PaladinBossHealthBarInstance = CreateWidget<UPaladinBossHealthBar>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PaladinBossMainWidget->GetClass());
        PaladinBossHealthBarInstance->AddToViewport();
    }
    else return;
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
        }
    }
}