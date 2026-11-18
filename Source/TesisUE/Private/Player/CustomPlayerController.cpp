#include "Player/CustomPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Entities/Entity.h"
#include "GAS/VelmaraAttributeSet.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GenericTeamAgentInterface.h"
#include "Blueprint/UserWidget.h"
#include "DataAssets/InputData.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"

#include "HUD/PlayerMainHUD.h"

namespace
{
    // AEntity::IsAlive() no sirve para enemigos (State.Dead nunca se aplica). Sin AttributeSet, Health daría 0: no cuenta como muerto.
    bool IsEntityDead(const AEntity* Entity)
    {
        const UAbilitySystemComponent* ASC = Entity ? Entity->GetAbilitySystemComponent() : nullptr;
        const FGameplayAttribute HealthAttribute = UVelmaraAttributeSet::GetHealthAttribute();
        return ASC && ASC->HasAttributeSetForAttribute(HealthAttribute) && ASC->GetNumericAttribute(HealthAttribute) <= 0.f;
    }
}


ACustomPlayerController::ACustomPlayerController()
{
    CurrentPauseMenuInstance = nullptr;
    CurrentOptionsMenuInstance_Pause = nullptr;
    bIsGamePausedExplicitly = false;

    PauseSoundMix = nullptr;
    PlayerMainHUD = nullptr;
    OriginalBody = nullptr;
    
    TeamId = FGenericTeamId(2);
}

bool ACustomPlayerController::PerformPossession(APawn* NewPawn)
{
    if (!NewPawn || NewPawn == GetPawn()) return false;

    if (!UTotemChargeComponent::CanActorBePossessed(NewPawn)) return false;

    APawn* PreviousPawn = GetPawn();
    if (!OriginalBody)
    {
        OriginalBody = PreviousPawn;
    }

    // Se guarda antes de Possess: al poseer, el AIController del enemigo queda suelto.
    AController* NewPawnPreviousController = NewPawn->GetController();
    const AEntity* NewEntity = Cast<AEntity>(NewPawn);
    const FGenericTeamId NewPawnTeam = NewEntity ? NewEntity->GetTeamId() : FGenericTeamId::NoTeam;

    UnPossess();

    Possess(NewPawn);

    if (UAbilitySystemComponent* NewASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn))
    {
        NewASC->InitAbilityActorInfo(this, NewPawn);
    }

    if (PreviousPawn && PreviousPawn != OriginalBody)
    {
        ReleaseHost(PreviousPawn);
    }

    if (NewPawn != OriginalBody)
    {
        HostAIController = NewPawnPreviousController;
        HostTeamId = NewPawnTeam;
        GetWorldTimerManager().SetTimer(PossessionTickTimer, this, &ACustomPlayerController::TickPossession, PossessionTickInterval, true);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(PossessionTickTimer);
        bEndingPossession = false;
    }

    return true;
}

void ACustomPlayerController::ReleaseHost(APawn* Host)
{
    if (UAbilitySystemComponent* HostASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Host))
    {
        HostASC->InitAbilityActorInfo(Host, Host);
    }

    AEntity* HostEntity = Cast<AEntity>(Host);
    if (!HostEntity) return; // Tótems: no tienen IA ni equipo que restaurar.

    HostEntity->TeamId = HostTeamId;

    // Se destruye el AIController viejo: re-poseer con él no reinicia el Behavior Tree de forma confiable.
    if (AController* OldController = HostAIController.Get())
    {
        OldController->Destroy();
    }
    HostAIController = nullptr;

    // Murió poseído: CBP_Enemy.HandleControllerDeactivation espera un AIController para apagar la IA y hacer ragdoll.
    if (IsEntityDead(HostEntity))
    {
        HostEntity->SpawnDefaultController();
        return;
    }

    // Aturdido: sin controller no hace nada hasta que vuelve la IA.
    TWeakObjectPtr<AEntity> WeakHost = HostEntity;
    FTimerHandle StunTimer;
    GetWorldTimerManager().SetTimer(StunTimer, [WeakHost]()
    {
        AEntity* Entity = WeakHost.Get();
        if (Entity && !IsEntityDead(Entity) && !Entity->GetController())
        {
            Entity->SpawnDefaultController();
        }
    }, FMath::Max(UnpossessStunDuration, 0.01f), false);
}

void ACustomPlayerController::TickPossession()
{
    if (!IsPossessing()) return;

    // El cuerpo poseído desapareció (pool/destroy): volver directo, no hay a quién mandarle el evento.
    if (!GetPawn())
    {
        if (OriginalBody) PerformPossession(OriginalBody);
        return;
    }

    if (bEndingPossession)
    {
        // El evento no sacó al jugador (p. ej. el cuerpo murió y bloquea habilidades): volver sin pasar por GA_Possess.
        PerformPossession(OriginalBody);
        return;
    }

    const AEntity* Host = Cast<AEntity>(GetPawn());
    UAbilitySystemComponent* BodyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OriginalBody);
    bool bShouldEnd = IsEntityDead(Host);

    if (BodyASC && Host) // Los tótems no consumen sangre.
    {
        const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
        const float NewEnergy = FMath::Max(0.f, BodyASC->GetNumericAttribute(EnergyAttribute) - PossessionEnergyDrainPerSecond * PossessionTickInterval);
        BodyASC->SetNumericAttributeBase(EnergyAttribute, NewEnergy);
        bShouldEnd |= NewEnergy <= 0.f;
    }

    if (bShouldEnd)
    {
        bEndingPossession = true;
        // GA_Possess del cuerpo poseído hace la vuelta completa (interfaz, tags, widget).
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), FGameplayTag::RequestGameplayTag(TEXT("Event.Posses.Return")), FGameplayEventData());
    }
}

bool ACustomPlayerController::IsPossessing() const
{
    return OriginalBody != GetPawn();
}

void ACustomPlayerController::BeginPlay()
{
    Super::BeginPlay();

    OriginalBody = GetPawn();
    
    PlayerMainHUD = Cast<APlayerMainHUD>(GetHUD());

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ACustomPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindAction("PauseGame", IE_Pressed, this, &ACustomPlayerController::TogglePauseMenu);
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        /*if (InputData->Inputs.MoveAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.MoveAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Input_Move);
            EnhancedInputComponent->BindAction(InputData->Inputs.MoveAction, ETriggerEvent::Completed, this, &ACustomPlayerController::Input_Move);
        }
		
        if (InputData->Inputs.LookAction)
            EnhancedInputComponent->BindAction(InputData->Inputs.LookAction, ETriggerEvent::Triggered, this, &ACustomPlayerController::Input_Look);*/

        if (InputData->Inputs.CrouchAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.CrouchAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Crouch);
            EnhancedInputComponent->BindAction(InputData->Inputs.CrouchAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Crouch);
        }
        
        if (InputData->Inputs.JumpAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Jump);
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Jump);
        }
        
        if (InputData->Inputs.DodgeAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Dodge);
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Dodge);
        }
        
        if (InputData->Inputs.InteractAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }

        if (InputData->Inputs.InventoryAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InventoryAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.InventoryAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }

        if (InputData->Inputs.ToggleWeaponAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.ToggleWeaponAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.ToggleWeaponAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }
        
        if (InputData->Inputs.AttackAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.AttackAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Attack);
            EnhancedInputComponent->BindAction(InputData->Inputs.AttackAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Attack);
        }

        if (InputData->Inputs.InputAction_Skill1)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill1);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill1);
        }

        if (InputData->Inputs.InputAction_Skill2)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill2);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill2);
        }

        if (InputData->Inputs.InputAction_Skill3)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill3);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill3);
        }

        if (InputData->Inputs.InputAction_Skill4)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill4);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill4);
        }

        if (InputData->Inputs.PossessAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.PossessAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Possess);
            EnhancedInputComponent->BindAction(InputData->Inputs.PossessAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Possess);
        }
    }
}

void ACustomPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    if (TeamId != NewTeamID)
    {
        TeamId = NewTeamID;
    }
}

FGenericTeamId ACustomPlayerController::GetGenericTeamId() const
{
    return TeamId;
}

ETeamAttitude::Type ACustomPlayerController::GetTeamAttitudeTowards(const AActor& Other) const
{
    const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
    if (!OtherTeamAgent) return ETeamAttitude::Neutral;

    const FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

    if (OtherTeamId == 255) return ETeamAttitude::Neutral;
    if (OtherTeamId == TeamId) return ETeamAttitude::Friendly;
    
    return ETeamAttitude::Hostile;
}

void ACustomPlayerController::AbilityInputTagPressed(EVelmaraAbilityInputID InputId)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                if (ASC)
                {
                    ASC->AbilityLocalInputPressed(static_cast<int32>(InputId));
                }
            }
        }
    }
}

void ACustomPlayerController::AbilityInputTagReleased(EVelmaraAbilityInputID InputId)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                if (ASC)
                {
                    ASC->AbilityLocalInputReleased(static_cast<int32>(InputId));
                }
            }
        }
    }
}

/*void ACustomPlayerController::Input_Move(const FInputActionValue& Value)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (ASI->GetAbilitySystemComponent())
            {
                const FVector2D FlatVector = Value.Get<FVector2D>();
	
                const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);

                const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
                const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

                ControlledPawn->AddMovementInput(DirectionForward, FlatVector.Y);
                ControlledPawn->AddMovementInput(DirectionSideward, FlatVector.X);

                if (Value.Get<bool>())
                {
                    InputMovingVector = FlatVector;
                }
                else
                {
                    InputMovingVector = FVector2D::ZeroVector;
                }
            }
        }
    }
}

void ACustomPlayerController::Input_Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
}*/

void ACustomPlayerController::TogglePauseMenu()
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
