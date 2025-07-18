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
#include "Enemy/Spectre.h"
#include "Enemy/Enemy.h"

#include "Items/Weapons/Sword.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Kismet/GameplayStatics.h"

#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include <SceneEvents/NewGameInstance.h>

#include "Animation/AnimInstance.h"


APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI = EAutoPossessAI::Disabled;

	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

	SpectralWeaponComponent = CreateDefaultSubobject<USpectralWeaponComponent>(TEXT("SpectralWeapon"));

	//GetCombatComponent()->SetAttackTokens(3);
}

void APlayerMain::PerformSpectralAttack_Implementation()
{
	if (IsEquipping() || GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_Unequipped) return;

	SpectralWeaponComponent->PrimaryFire();
}

void APlayerMain::PerformSpectralBarrier_Implementation()
{
	if (IsEquipping() || GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_Unequipped) return;

	SpectralWeaponComponent->SecondaryFire();
}

void APlayerMain::ResetSpectralAttack_Implementation()
{
	SpectralAttackIndex = 0;
	GetCombatComponent()->bIsSaveLightAttack = false;
}

void APlayerMain::GetHit_Implementation(AEntity* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived)
{
	Super::GetHit_Implementation(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

	if (DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		StunBehavior();
	}	
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	GetAttributeComponent()->RegenerateTick();
	GetAttributeComponent()->OnEntityDead.AddDynamic(this, &APlayerMain::PerformDead);

	if (!GetCombatComponent()->OnWallHit.IsBound())
	{
		GetCombatComponent()->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);
	}
	
	GetPossessionComponent()->OnPossessionAttemptFailed.AddDynamic(GetCombatComponent(), &UCombatComponent::Input_Execute);

	GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral ?
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

	if (APlayerController* PlayerController = CastChecked<APlayerController>(GetController()))
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

void APlayerMain::SearchTarget()
{
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() + GetViewRotation().Vector() * TrackTargetDistance;

	AActor* Enemy = GetCombatComponent()->SphereTraceForEnemies(Start, End);

	if (Enemy)
	{
		SpectralTarget = Cast<ASpectre>(Enemy);
	}
	else SpectralTarget = nullptr;
}

float APlayerMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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

void APlayerMain::Equipping(bool bIsSwordBeingEquipped)
{
	ECharacterForm ActualForm = GetCharacterStateComponent()->GetCurrentCharacterState().Form;

	if (ActualForm == ECharacterForm::ECF_Human) //si estoy en humano
	{
		/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("Human form"));*/

		if (bIsSwordBeingEquipped) //y estoy equipando una espada
		{
			if (GetInventoryComponent()->GetEquippedItem())
			{
				if (SpectralWeaponComponent->bWasInitialized && GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_EquippedPistol)
				{
					PlayAnimMontage(EquipPistolMontage, 1.f, FName("BackToSword"));
					
				}
				else
				{
					/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, FString("Equipping sword"));*/

					PlayAnimMontage(EquipSwordMontage, 1.f, FName("Equip"));
					GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippingSword);
				}
			}
			else
			{
				if (SpectralWeaponComponent->bWasInitialized)
				{
					PlayAnimMontage(EquipPistolMontage, 1.f, FName("Unequip"));
					GetCharacterStateComponent()->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
				}
				/*else GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan, FString("No equipped item, so none animation will be played..."));*/
			}
		}
		else //o estoy equipando una pistola
		{
			if (!GetInventoryComponent()->GetEquippedItem()) //si no tengo espada
			{
				/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange, FString("Only pick pistol"));*/

				PlayAnimMontage(EquipPistolMontage, 1.f, FName("Unequip"));
				GetCharacterStateComponent()->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
			}
			else
			{
				//si tengo espada equipada
				/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("Unequip sword and equip pistol"));*/

				PlayAnimMontage(EquipSwordMontage, 1.f, FName("PickPistol"));
				GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippingSword);
			}
		}
	}
	else //si no estoy en humano
	{
		/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Purple, FString("Spectral form"));*/

		if (!bIsSwordBeingEquipped)
		{
			if (GetInventoryComponent()->GetEquippedItem())
			{
				if (SpectralWeaponComponent->bWasInitialized)
				{
					if (GetCharacterStateComponent()->GetCurrentCharacterState().State == ECharacterStates::ECS_EquippedSword)
					{
						/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan, FString("Has sword equipped"));*/

						PlayAnimMontage(EquipSwordMontage, 1.f, FName("SwitchToPistol"));
						GetCharacterStateComponent()->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
					}
					else
					{
						/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Blue, FString("Sword it's on the back"));*/

						PlayAnimMontage(EquipPistolMontage, 1.f, FName("Equip"));
						GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_Unequipped);
						GetCharacterStateComponent()->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
					}
				}
				else
				{
					PlayAnimMontage(EquipSwordMontage, 1.f, FName("Unequip"));
					GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippingSword);
				}
			}
			else
			{
				if (SpectralWeaponComponent->bWasInitialized)
				{
					/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Blue, FString("Sword not equipped"));*/

					PlayAnimMontage(EquipPistolMontage, 1.f, FName("Equip"));
					GetCharacterStateComponent()->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
				}
			}
			
		}
	}	
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

void APlayerMain::Die(UAnimMontage* DeathAnim, FName Section)
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
	if (GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead)
	{
		StopAnimMontage();

		if (PlayerControllerRef)
		{
			EnableInput(PlayerControllerRef);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Nothing);
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Nothing);
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
	if (GetCharacterStateComponent()->IsFormEqualToAny({ ECharacterForm::ECF_Spectral })) return;

	GetInventoryComponent()->ChangeWeapon(0);
	Equipping(true);
}

void APlayerMain::ChangeSecondaryWeapon()
{
	if (GetCharacterStateComponent()->IsFormEqualToAny({ ECharacterForm::ECF_Spectral })) return;

	GetInventoryComponent()->ChangeWeapon(1);
	Equipping(true);
}