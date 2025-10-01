#include "Player/PlayerMain.h"

#include "SceneEvents/VelmaraGameModeBase.h"
#include "SceneEvents/VelmaraGameStateBase.h"
#include "SceneEvents/VelmaraGameInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/ChangeModeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/PossessionComponent.h"
#include "Curves/CurveFloat.h"

#include "Camera/CameraActor.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GenericTeamAgentInterface.h"

#include "Kismet/GameplayStatics.h"

#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include "DataAssets/InputData.h"
#include "DataAssets/MontagesData.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInput/Public/InputMappingContext.h"

APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI = EAutoPossessAI::Disabled;

	ChangeModeComponent = CreateDefaultSubobject<UChangeModeComponent>(TEXT("ChangeModeComponent"));

	GetAttributeComponent()->OnEntityDead.AddDynamic(this, &APlayerMain::PerformDead);
	GetAttributeComponent()->OnOutOfEnergy.AddDynamic(this, &APlayerMain::ApplyHumanMode);
	
	GetPossessionComponent()->OnPossessionAttemptSucceed.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StartDecreaseEnergy);
	GetPossessionComponent()->OnPossessionReleased.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StopDecreaseEnergy);

	ChangeModeComponent->OnHumanEffectApplied.AddDynamic(this, &APlayerMain::ApplyHumanMode);
	ChangeModeComponent->OnSpectralEffectApplied.AddDynamic(this, &APlayerMain::ApplySpectralMode);

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC(TEXT("/Game/Blueprints/Player/Input"));
	if (IMC.Succeeded())
	{
		CharacterContext = IMC.Object.Get();
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "IMC Found!");
	}
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	ChangeModeComponent->OnHumanEffectApplied.AddDynamic(this, &APlayerMain::ApplyHumanMode);
	ChangeModeComponent->OnSpectralEffectApplied.AddDynamic(this, &APlayerMain::ApplySpectralMode);
	
	if (const APlayerController* PlayerController = CastChecked<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			Subsystem->AddMappingContext(CharacterContext, 0);
}

void APlayerMain::PerformDead()
{
	Die(MontagesData->Montages.DeathMontage, NAME_None);
}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_SwitchForm, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Possess, ETriggerEvent::Started, this, &APlayerMain::Input_Ability);
	}
}

float APlayerMain::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!bCanReceiveDamage) return 0.f;

	if (GetAttributeComponent() && GetAttributeComponent()->IsAlive())
	{
		GetAttributeComponent()->ReceiveDamage(DamageAmount);
	}
	else
	{
		Die(MontagesData->Montages.DeathMontage, NAME_None);
	}
	return DamageAmount;
}

void APlayerMain::ToggleForm()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Nothing, ECharacterActionsStates::ECAS_Stun }))
	{
		ChangeModeComponent->ToggleForm();
	}
}

void APlayerMain::Die(UAnimMontage* DeathAnim, const FName Section)
{
	Super::Die(DeathAnim, Section);

	if (PlayerControllerRef)
	{
		DisableInput(PlayerControllerRef);
	}

	FTimerHandle TimerHandle_LoadCheckpoint;
	GetWorldTimerManager().SetTimer(TimerHandle_LoadCheckpoint, this, &APlayerMain::LoadLastCheckpoint, 2.0f, false);
}

void APlayerMain::Revive()
{
	if (Execute_GetCharacterStateComponent(this)->CurrentStates.Action == ECharacterActionsStates::ECAS_Dead)
	{
		Execute_StopAnimMontage(this, GetCurrentMontage());

		if (PlayerControllerRef)
		{
			EnableInput(PlayerControllerRef);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Execute_GetCharacterStateComponent(this)->SetAction(ECharacterActionsStates::ECAS_Nothing);
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		Execute_GetCharacterStateComponent(this)->SetAction(ECharacterActionsStates::ECAS_Nothing);
		FollowCamera->AttachToComponent(GetSpringArmComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<AVelmaraGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::LoadLastCheckpoint() const
{
	if (UVelmaraGameInstance* GameInst = GetGameInstance<UVelmaraGameInstance>())
	{
		GameInst->LoadPlayerProgress(GameInst->ActiveSaveSlotIndex);
	}
}

void APlayerMain::ApplyHumanMode()
{
	if (CharacterStateComponent->IsModeEqualToAny({ECharacterModeStates::ECMS_Human})) return;
	
	if (const TScriptInterface<IGenericTeamAgentInterface> TeamAgent = GetController())
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId(2));
	}
	
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;

	if (Execute_GetCurrentWeapon(this))
	{
		Execute_GetCurrentWeapon(this)->EnableVisuals();
		CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	}

	if (GetPossessionComponent()->GetPossessedEntity())
	{
		GetPossessionComponent()->ReleasePossession();
	}

	CharacterStateComponent->SetMode(ECharacterModeStates::ECMS_Human);

	SetCombatStrategy(ECharacterModeStates::ECMS_Human);

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "HUMAN");
}

void APlayerMain::ApplySpectralMode()
{
	if (CharacterStateComponent->IsModeEqualToAny({ECharacterModeStates::ECMS_Spectral})) return;
	
	if (const TScriptInterface<IGenericTeamAgentInterface> TeamAgent = GetController())
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId(0));
	}
	
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = true;

	if (Execute_GetCurrentWeapon(this))
	{
		Execute_GetCurrentWeapon(this)->DisableVisuals();
		CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_Unequipped);
	}
	
	CharacterStateComponent->SetMode(ECharacterModeStates::ECMS_Spectral);

	SetCombatStrategy(ECharacterModeStates::ECMS_Spectral);

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "VAMPIRE");
}
