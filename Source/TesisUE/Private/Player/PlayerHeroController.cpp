#include "Player/PlayerHeroController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "DataAssets/InputData.h"
#include "Entities/Entity.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"

#include "HUD/PlayerMainHUD.h"


APlayerHeroController::APlayerHeroController()
{
    CurrentPauseMenuInstance = nullptr;
    CurrentOptionsMenuInstance_Pause = nullptr;
    bIsGamePausedExplicitly = false;

    PauseSoundMix = nullptr;
    PlayerMainHUD = nullptr;
    OriginalBody = nullptr;
}

void APlayerHeroController::PerformPossession(APawn* NewPawn)
{
    if (!NewPawn || NewPawn == GetPawn()) return;

    if (!OriginalBody)
    {
        OriginalBody = GetPawn();
    }

    UnPossess();

    Possess(NewPawn);

    if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(NewPawn))
    {
        if (UAbilitySystemComponent* NewASC = ASI->GetAbilitySystemComponent())
        {
            NewASC->InitAbilityActorInfo(this, NewPawn);
        }
    }
}

void APlayerHeroController::BeginPlay()
{
    Super::BeginPlay();

    PlayerMainHUD = Cast<APlayerMainHUD>(GetHUD());

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void APlayerHeroController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction("PauseGame", IE_Pressed, this, &APlayerHeroController::TogglePauseMenu);
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (InputData->Inputs.MoveAction)
            EnhancedInputComponent->BindAction(InputData->Inputs.MoveAction, ETriggerEvent::Triggered, this, &APlayerHeroController::Input_Move);
		
        if (InputData->Inputs.LookAction)
            EnhancedInputComponent->BindAction(InputData->Inputs.LookAction, ETriggerEvent::Triggered, this, &APlayerHeroController::Input_Look);

        if (InputData->Inputs.JumpAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Jump);
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Jump);
        }

        if (InputData->Inputs.DodgeAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Dodge);
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Dodge);
        }

        if (InputData->Inputs.InteractAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }
        
        if (InputData->Inputs.PrimaryAttackAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.PrimaryAttackAction, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::PrimaryAttack);
            EnhancedInputComponent->BindAction(InputData->Inputs.PrimaryAttackAction, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::PrimaryAttack);
        }

        if (InputData->Inputs.SecondaryAttackAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.SecondaryAttackAction, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::SecondaryAttack);
            EnhancedInputComponent->BindAction(InputData->Inputs.SecondaryAttackAction, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::SecondaryAttack);
        }

        if (InputData->Inputs.InputAction_Skill1)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill1);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill1);
        }

        if (InputData->Inputs.InputAction_Skill2)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill2);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill2);
        }

        if (InputData->Inputs.InputAction_Skill3)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill3);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill3);
        }

        if (InputData->Inputs.InputAction_Skill4)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Started, this, &APlayerHeroController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill4);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Completed, this, &APlayerHeroController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill4);
        }
    }
}

void APlayerHeroController::AbilityInputTagPressed(EVelmaraAbilityInputID InputId)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                ASC->AbilityLocalInputPressed(static_cast<int32>(InputId));
            }
        }
    }
}

void APlayerHeroController::AbilityInputTagReleased(EVelmaraAbilityInputID InputId)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                ASC->AbilityLocalInputReleased(static_cast<int32>(InputId));
            }
        }
    }
}

void APlayerHeroController::Input_Move(const FInputActionValue& Value)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (const UAbilitySystemComponent* AbilitySystemComponent = ASI->GetAbilitySystemComponent())
            {
                //const bool CanPerformMovement = AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Dead)
                //|| AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Blocking)
                //|| AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Executing);
                //
                //if (!CanPerformMovement) return;
                
                const FVector2D FlattenedVector = Value.Get<FVector2D>();
	
                const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);

                const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
                const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

                ControlledPawn->AddMovementInput(DirectionForward, FlattenedVector.Y);
                ControlledPawn->AddMovementInput(DirectionSideward, FlattenedVector.X);
            }
        }
    }
}

void APlayerHeroController::Input_Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
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
            return;
        }

        CurrentPauseMenuInstance = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
        if (CurrentPauseMenuInstance)
        {
            CurrentPauseMenuInstance->AddToViewport();
            SetShowMouseCursor(true);

            FInputModeGameAndUI InputModeData;
            InputModeData.SetWidgetToFocus(CurrentPauseMenuInstance->TakeWidget()); // Enfocar el men� de pausa
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
