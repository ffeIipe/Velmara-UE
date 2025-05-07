// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerHeroController.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

APlayerHeroController::APlayerHeroController()
{
	HeroTeamID = FGenericTeamId(0);

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
    // Si asignaste las clases en el constructor o desde un Blueprint derivado, aquí ya deberían estar disponibles.
}

void APlayerHeroController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction("PauseGame", IE_Pressed, this, &APlayerHeroController::TogglePauseMenu);
        // Asegúrate de tener una acción "PauseGame" en Project Settings > Input
        // y asígnale una tecla (ej. Escape).
    }
}

void APlayerHeroController::TogglePauseMenu()
{
    if (bIsGamePausedExplicitly) // Si estamos pausados, vamos a despausar
    {
        if (CurrentOptionsMenuInstance_Pause && CurrentOptionsMenuInstance_Pause->IsInViewport())
        {
            CurrentOptionsMenuInstance_Pause->RemoveFromParent();
            CurrentOptionsMenuInstance_Pause = nullptr;
            // No despausamos el juego todavía, solo cerramos opciones. El menú de pausa sigue activo.
            // Hacemos que el menú de pausa recupere el foco
            if (CurrentPauseMenuInstance)
            {
                CurrentPauseMenuInstance->SetFocus(); // O FInputModeUIOnly con el menú de pausa como widget a enfocar
            }
            return; // Salimos para no despausar el juego si solo estábamos cerrando opciones
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
    else // Si no estamos pausados, vamos a pausar
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

// void AMyPlayerController::ShowOptionsMenuFromPause()
// {
//     if (OptionsMenuWidgetClass_Pause && !CurrentOptionsMenuInstance_Pause)
//     {
//         CurrentOptionsMenuInstance_Pause = CreateWidget<UWBP_OptionsMenu>(this, OptionsMenuWidgetClass_Pause);
//         if (CurrentOptionsMenuInstance_Pause)
//         {
//             CurrentOptionsMenuInstance_Pause->AddToViewport(10); // ZOrder alto para que esté encima del menú de pausa
             // Podrías querer que el menú de opciones tome el foco aquí
//         }
//     }
// }

FGenericTeamId APlayerHeroController::GetGenericTeamId() const
{
	return HeroTeamID;
}