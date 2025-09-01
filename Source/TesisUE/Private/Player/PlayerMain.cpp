#include "Player/PlayerMain.h"

#include "EngineUtils.h"
#include "SceneEvents/NewGameModeBase.h"
#include "SceneEvents/NewGameStateBase.h"
#include "SceneEvents/NewGameInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/PlayerFormComponent.h"
#include "Components/MementoComponent.h"
#include "Components/CombatComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/PossessionComponent.h"
#include "Components/SpectralWeaponComponent.h"
#include "Curves/CurveFloat.h"

#include "Camera/CameraActor.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Kismet/GameplayStatics.h"

#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include "Interfaces/Weapon/WeaponInterface.h"

APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI = EAutoPossessAI::Disabled;

	GetAttributeComponent()->OnEntityDead.AddDynamic(this, &APlayerMain::PerformDead);
	
	GetPossessionComponent()->OnPossessionAttemptFailed.AddDynamic(GetCombatComponent(), &UCombatComponent::Input_Execute);
	GetPossessionComponent()->OnPossessionAttemptSucceed.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StartDecreaseEnergy);
	GetPossessionComponent()->OnPossessionReleased.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StopDecreaseEnergy);
	
	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));
	PlayerFormComponent->OnHumanEffectApplied.AddDynamic(this, &APlayerMain::ApplyHumanMode);
	PlayerFormComponent->OnSpectralEffectApplied.AddDynamic(this, &APlayerMain::ApplySpectralEffect);
	
	SpectralWeaponComponent = CreateDefaultSubobject<USpectralWeaponComponent>(TEXT("SpectralWeapon"));
}

void APlayerMain::GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	FDamageEvent const& DamageEvent, const float DamageReceived)
{
	Super::GetHit(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

	if (DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		StunBehavior();
	}	
}

// void APlayerMain::PerformSpectralAttack_Implementation()
// {
// 	if (IsEquipping() || GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_Unequipped) return;
//
// 	SpectralWeaponComponent->PrimaryFire();
// }
//
// void APlayerMain::PerformSpectralBarrier_Implementation()
// {
// 	if (IsEquipping() || GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_Unequipped) return;
//
// 	SpectralWeaponComponent->SecondaryFire();
// }
//
// void APlayerMain::ResetSpectralAttack_Implementation()
// {
// 	SpectralAttackIndex = 0;
// 	GetCombatComponent()->bIsSaveLightAttack = false;
// }

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	if (!GetCombatComponent()->OnWallHit.IsBound())
	{
		GetCombatComponent()->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);
	}
	
	
	GetCharacterStateComponent()->CurrentStates.Mode == ECharacterMode::ECM_Spectral ?
		SpectralWeaponComponent->EnableSpectralWeapon(true) : SpectralWeaponComponent->EnableSpectralWeapon(false);

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		FollowCamera = *It;
		break;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()); FollowCamera && PC)
	{
		FollowCamera->AttachToComponent(GetSpringArmComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SpringEndpoint"));
		PC->SetViewTargetWithBlend(FollowCamera, 1.f);
	}

	if (const APlayerController* PlayerController = CastChecked<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			Subsystem->AddMappingContext(CharacterContext, 0);

	if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
		{
			if (GetMementoComponent())
			{
				NewGameStateBase->RegisterMementoEntity(this);
			}
		}
	}
}

void APlayerMain::PerformDead()
{
	Die(DeathMontage, NAME_None);
}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputAction_SwitchForm, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(InputAction_Possess, ETriggerEvent::Started, GetPossessionComponent(), &UPossessionComponent::AttemptPossession);
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
		Die(DeathMontage, NAME_None);
	}
	return DamageAmount;
}

bool APlayerMain::IsPossessed()
{
	return true; //true by default, because it wouldn't be marked as a threat if not
}

void APlayerMain::ToggleForm()
{
	if (IsEquipping()) return;

	if (GetCharacterStateComponent()->IsActionEqualToAny({
		ECharacterActions::ECA_Dead,
		ECharacterActions::ECA_Block,
		ECharacterActions::ECA_Finish,
		ECharacterActions::ECA_Attack,
		ECharacterActions::ECA_Stun })) return;
	
	PlayerFormComponent->ToggleForm();
}

void APlayerMain::OutOfEnergy()
{
	Super::OutOfEnergy();

	PlayerFormComponent->ToggleForm();
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
	if (GetCharacterStateComponent()->CurrentStates.Action == ECharacterActions::ECA_Dead)
	{
		StopAnimMontage();

		if (PlayerControllerRef)
		{
			EnableInput(PlayerControllerRef);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterStateComponent()->SetAction(ECharacterActions::ECA_Nothing);
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		GetCharacterStateComponent()->SetAction(ECharacterActions::ECA_Nothing);
		FollowCamera->AttachToComponent(GetSpringArmComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::LoadLastCheckpoint() const
{
	if (UNewGameInstance* GameInst = GetGameInstance<UNewGameInstance>())
	{
		GameInst->LoadPlayerProgress(GameInst->ActiveSaveSlotIndex);
	}
}

void APlayerMain::ChangePrimaryWeapon()
{
	if (GetCharacterStateComponent()->IsModeEqualToAny({ ECharacterMode::ECM_Spectral })) return;

	GetInventoryComponent()->ChangeWeapon(0);
}

void APlayerMain::ChangeSecondaryWeapon()
{
	if (GetCharacterStateComponent()->IsModeEqualToAny({ ECharacterMode::ECM_Spectral })) return;

	GetInventoryComponent()->ChangeWeapon(1);
}

void APlayerMain::ApplyHumanMode()
{
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;

	if (GetWeaponEquipped())
	{
		GetWeaponEquipped()->EnableVisuals(true);
	}

	if (GetPossessionComponent()->GetPossessedEntity())
	{
		GetPossessionComponent()->ReleasePossession();
	}

	GetCharacterStateComponent()->SetMode(ECharacterMode::ECM_Human);
}

void APlayerMain::ApplySpectralEffect()
{
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = true;
	GetCharacterStateComponent()->SetMode(ECharacterMode::ECM_Spectral);
}
