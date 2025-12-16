#include "Player/PlayerMain.h"

#include "EngineUtils.h"
#include "SceneEvents/NewGameModeBase.h"
#include "SceneEvents/NewGameStateBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/PlayerFormComponent.h"
#include "Components/BoxComponent.h"
#include "Components/MementoComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/SpectralWeaponComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/PossessionComponent.h"
#include "Curves/CurveFloat.h"

#include "Camera/CameraActor.h"
#include "Enemy/Spectre.h"
#include "Enemy/Enemy.h"
#include "Enemy/Paladin.h"

#include "Items/Weapons/Sword.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "SpectralMode/Interfaces/SpectralInteractable.h"

#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include <SceneEvents/NewGameInstance.h>

#include "Animation/AnimInstance.h"
#include <Enemy/Paladin/PaladinBoss.h>


APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

	SpectralWeaponComponent = CreateDefaultSubobject<USpectralWeaponComponent>(TEXT("SpectralWeapon"));
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

void APlayerMain::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	Super::GetHit_Implementation(DamageCauser, ImpactPoint, DamageType, DamageReceived);

	if (DamageType == USpectralTrapDamageType::StaticClass())
	{
		StunBehavior();
	}	
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	GetAttributeComponent()->RegenerateTick();

	GetAttributeComponent()->OnEntityDead.AddLambda(
		[this]
		{
			Die();
		}
	);

	GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral ?
		SpectralWeaponComponent->EnableSpectralWeapon(true) : SpectralWeaponComponent->EnableSpectralWeapon(false);

	GetCombatComponent()->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		FollowCamera = *It;
		break;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (FollowCamera && PC)
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
		Die();
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
					GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippingSword);
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
	if (!IsValid(GetCharacterStateComponent())) return;
	
	if (!GetWorld()) return;

	if (!IsValid(GetAttributeComponent())) return;

	if (IsEquipping()) return;

	if (GetCharacterStateComponent()->IsActionEqualToAny({
		ECharacterActions::ECA_Dead,
		ECharacterActions::ECA_Block,
		ECharacterActions::ECA_Finish,
		ECharacterActions::ECA_Attack,
		ECharacterActions::ECA_Stun })
		)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastTransformationTime < TransformationCooldown) return;

	if (GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
	{
		GetAttributeComponent()->StartDecreaseEnergy();
		WithEnergy();
		GetAttributeComponent()->StopRegenerateTick();
	}
	else
	{
		GetAttributeComponent()->StopDecreaseEnergy();
		OutOfEnergy();
		GetAttributeComponent()->RegenerateTick();
	}

	LastTransformationTime = CurrentTime;
}

void APlayerMain::WithEnergy()
{
	if (GetAttributeComponent()->ItHasEnergy())
	{
		PlayerFormComponent->ToggleForm(true);
		GetAttributeComponent()->StartDecreaseEnergy();
		GetAttributeComponent()->OnDepletedCallback = [this]() { OutOfEnergy(); };
		GetAttributeComponent()->RegenerateTick();
		GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = true;
	}
}

void APlayerMain::OutOfEnergy()
{
	PlayerFormComponent->ToggleForm(false);
	if (GetAttributeComponent())
	{
		GetAttributeComponent()->RegenerateTick();
	}

	if (GetCharacterMovement() && GetCharacterMovement()->GetPawnOwner())
	{
		GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;
	}

	if (SpectralWeaponComponent)
	{
		SpectralWeaponComponent->EnableSpectralWeapon(false);
	}

	if (GetInventoryComponent() && GetInventoryComponent()->GetEquippedItem())
	{
		GetInventoryComponent()->GetEquippedItem()->EnableVisuals(true);
	}

	if (GetPossessionComponent()->GetEnemyPossessed())
	{
		GetPossessionComponent()->ReleasePossession();
	}
}

void APlayerMain::Die()
{
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

void APlayerMain::LoadLastCheckpoint()
{
	UNewGameInstance* GameInst = GetGameInstance<UNewGameInstance>();
	if (GameInst)
	{
		GameInst->LoadPlayerProgress(GameInst->ActiveSaveSlotIndex);
	}

	UGameplayStatics::OpenLevel(this, FName("AugusTest"));
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

//void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
//	{
//		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
//		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
//		EnhancedInputComponent->BindAction(GetExtraMovementComponent()->JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
//		EnhancedInputComponent->BindAction(GetExtraMovementComponent()->DodgeAction, ETriggerEvent::Started, this, &APlayerMain::Dodge);
//		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerMain::Interact);
//
//		EnhancedInputComponent->BindAction(GetCombatComponent()->AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
//		EnhancedInputComponent->BindAction(GetCombatComponent()->HeavyAttackAction, ETriggerEvent::Started, this, &APlayerMain::HeavyAttack);
//		EnhancedInputComponent->BindAction(GetCombatComponent()->LaunchAction, ETriggerEvent::Triggered, this, &APlayerMain::LaunchAttack);
//		//EnhancedInputComponent->BindAction(GetCombatComponent()->BlockAction, ETriggerEvent::Started, this, &APlayerMain::Block);
//		//EnhancedInputComponent->BindAction(GetCombatComponent()->BlockAction, ETriggerEvent::Completed, this, &APlayerMain::ReleaseBlock);
//
//		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
//		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Completed, this, &APlayerMain::PossessEnemy);
//		
//		//EnhancedInputComponent->BindAction(GetInventoryComponent()->Slot1_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangePrimaryWeapon);
//		//EnhancedInputComponent->BindAction(GetInventoryComponent()->Slot2_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangeSecondaryWeapon);
//	}
//}