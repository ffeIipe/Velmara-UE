// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerHeroController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "HUD/PlayerMainHUD.h"
#include "HUD/PlayerMainWidget.h"
#include "HUD/PaladinBossHealthBar.h"
#include <Player/PlayerMain.h>

APlayerHeroController::APlayerHeroController()
{
	HeroTeamID = FGenericTeamId(2);

    CurrentPauseMenuInstance = nullptr;
    CurrentOptionsMenuInstance_Pause = nullptr;
    bIsGamePausedExplicitly = false;

    // Ejemplo de cómo asignar clases desde C++ (alternativa a EditDefaultsOnly en Blueprints)
    // static ConstructorHelpers::FClassFinder<UUserWidget> PauseMenuAsset(TEXT("/Game/UI/WBP_PauseMenu")); // Cambia la ruta
    // if (PauseMenuAsset.Succeeded())
    // {
    //     PauseMenuWidgetClass = PauseMenuAsset.Class;
    // }

    // static ConstructorHelpers::FClassFinder<USoundMix> PauseSoundMixAsset(TEXT("/Game/Audio/SM_PauseAttenuation")); // Cambia la ruta
    // if (PauseSoundMixAsset.Succeeded())
    // {
    //     PauseSoundMix = PauseSoundMixAsset.Object;
    // }
}

void APlayerHeroController::BeginPlay()
{
    Super::BeginPlay();

    PlayerMainHUD = Cast<APlayerMainHUD>(GetHUD());
}

void APlayerHeroController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction("PauseGame", IE_Pressed, this, &APlayerHeroController::TogglePauseMenu);
    }
}

void APlayerHeroController::TogglePauseMenu()
{
    if (bIsGamePausedExplicitly)
    {
        if (CurrentOptionsMenuInstance_Pause && CurrentOptionsMenuInstance_Pause->IsInViewport())
        {
            CurrentOptionsMenuInstance_Pause->RemoveFromParent();
            CurrentOptionsMenuInstance_Pause = nullptr;

            if (CurrentPauseMenuInstance)
            {
                CurrentPauseMenuInstance->SetFocus();
            }
            return;
        }

        if (CurrentPauseMenuInstance)
        {
            CurrentPauseMenuInstance->RemoveFromParent();
            CurrentPauseMenuInstance = nullptr;
        }

        SetShowMouseCursor(false);
        SetInputMode(FInputModeGameOnly());
        UGameplayStatics::SetGamePaused(GetWorld(), false);
        bIsGamePausedExplicitly = false;

        if (PauseSoundMix)
        {
            UGameplayStatics::PopSoundMixModifier(GetWorld(), PauseSoundMix);
        }
        UE_LOG(LogTemp, Log, TEXT("Game Unpaused"));
    }
    else
    {
        if (!PauseMenuWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("PauseMenuWidgetClass not set in PlayerController!"));
            return;
        }

        if (CurrentPauseMenuInstance && CurrentPauseMenuInstance->IsInViewport())
        {
            // Ya está visible, no debería pasar si la lógica es correcta, pero por si acaso
            return;
        }

        CurrentPauseMenuInstance = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
        if (CurrentPauseMenuInstance)
        {
            CurrentPauseMenuInstance->AddToViewport();
            SetShowMouseCursor(true);

            FInputModeGameAndUI InputModeData;
            InputModeData.SetWidgetToFocus(CurrentPauseMenuInstance->TakeWidget()); // Enfocar el menú de pausa
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputModeData);

            UGameplayStatics::SetGamePaused(GetWorld(), true);
            bIsGamePausedExplicitly = true;

            if (PauseSoundMix)
            {
                UGameplayStatics::PushSoundMixModifier(GetWorld(), PauseSoundMix);
            }
            UE_LOG(LogTemp, Log, TEXT("Game Paused"));
        }
    }
}

void APlayerHeroController::SetGenericTeamId(const FGenericTeamId& TeamID)
{
    HeroTeamID = TeamID;
}

FGenericTeamId APlayerHeroController::GetGenericTeamId() const
{
	return HeroTeamID;
}

void APlayerHeroController::HandleBossHealth(float HealthHP, float ShieldHP)
{
    if (PlayerMainHUD && PlayerMainHUD->PlayerMainWidgetInstance)
    {
        PlayerMainHUD->SetPaladinBossHealthBar();
        PlayerMainHUD->PaladinBossHealthBarInstance->ReceivePercentagesFromBoss(HealthHP, ShieldHP);
    }
}

void APlayerHeroController::ToggleInput(bool Bool)
{
    if (Bool)
    {
        if (APlayerMain* PlayerRef = Cast<APlayerMain>(GetPawn()))
        {
            PlayerRef->GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Nothing);
        }
    }
    else
    {
        if (APlayerMain* PlayerRef = Cast<APlayerMain>(GetPawn()))
        {
            PlayerRef->GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Stun);
        }
    }
}