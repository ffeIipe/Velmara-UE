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
#include "Components/CapsuleComponent.h"
#include "Components/MementoComponent.h"
#include "Components/CombatComponent.h"
#include "Components/InventoryComponent.h"
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

APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
	
	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttibuteComponent"));

	MementoComponent = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
}

ECharacterForm APlayerMain::GetCharacterForm_Implementation()
{
	return PlayerFormComponent->GetCharacterForm();
}

void APlayerMain::PerformSpectralAttack_Implementation()
{
	if (CombatComponent->GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		CombatComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
		SearchTarget();
		PlayAnimMontage(SpectralAttackCombo[SpectralAttackIndex]);

		SpectralAttackIndex++;

		if (SpectralAttackIndex >= SpectralAttackCombo.Num())
		{
			SpectralAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformSpectralBarrier_Implementation()
{
	if (CombatComponent->GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		CombatComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
		PlayAnimMontage(SpectralHeavyAttack);
	}
}

void APlayerMain::ResetSpectralAttack_Implementation()
{
	SpectralAttackIndex = 0;
	CombatComponent->bIsSaveLightAttack = false;
}

void APlayerMain::GetHit_Implementation(const FVector& ImpactPoint)
{

}

void APlayerMain::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (InventoryComponent)
	{
		AItem* CurrentItem = InventoryComponent->GetEquippedItem();
		if (CurrentItem)
		{
			UPrimitiveComponent* ItemCollisionComponent = CurrentItem->GetCollisionComponent();
			if (ItemCollisionComponent)
			{
				ItemCollisionComponent->SetCollisionEnabled(CollisionEnabled);
				if (ASword* Sword = Cast<ASword>(CurrentItem))
				{
					if (CollisionEnabled != ECollisionEnabled::NoCollision)
					{
						Sword->IgnoreActors.Empty();
					}
				}
			}
		}
	}
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	Attributes->RegenerateTick();
	
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		FollowCamera = *It;
		break;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (FollowCamera && PC)
	{
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SprinEndpoint"));
		PC->SetViewTargetWithBlend(FollowCamera, 1.f);
	}

	if (APlayerController* PlayerController = CastChecked<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			Subsystem->AddMappingContext(CharacterContext, 0);

	if (BufferCurve)
	{
		FOnTimelineFloat ProgressDodgeFunction;
		ProgressDodgeFunction.BindUFunction(this, FName("UpdateDodgeBuffer"));
		BufferDodgeTimeline->AddInterpFloat(BufferCurve, ProgressDodgeFunction);
	}

	if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
		{
			if (MementoComponent)
			{
				NewGameStateBase->RegisterMementoEntity(this);
			}
		}
	}
}

void APlayerMain::PerformDodge()
{
	if (CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	FVector MovementInput = GetLastMovementInputVector();
	if (!MovementInput.IsNearlyZero())
	{
		FRotator LookRotation = MovementInput.Rotation();
		SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
	}

	StopDodgeBufferEvent();
	DodgeBufferEvent(BufferDodgeDistance);
	CombatComponent->SetCharacterAction(ECharacterActions::ECA_Dodge);

	if (PlayerFormComponent && PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		PlayAnimMontage(DodgeMontage);
	}
	else
	{
		//directional animation based on last movement input vector
		PlayAnimMontage(SpectralDodgeMontage);
	}
}

void APlayerMain::DodgeBufferEvent(float BufferAmount)
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->PlayFromStart();
	}
}

void APlayerMain::StopDodgeBufferEvent()
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->Stop();
	}
}

void APlayerMain::UpdateDodgeBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferDodgeDistance);
}

void APlayerMain::UpdateBuffer(float Alpha, float BufferDistance)
{
	FVector CurrentLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();

	FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	SetActorLocation(TargetLocation, true);
}

void APlayerMain::SearchTarget()
{
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() + GetViewRotation().Vector() * TrackTargetDistance;

	AActor* Enemy = CombatComponent->SphereTraceForEnemies(Start, End);

	if (Enemy)
	{
		SpectralTarget = Cast<ASpectre>(Enemy);
	}
	else SpectralTarget = nullptr;
}

float APlayerMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//TODO: player healthbar and fx to receive damage

	if (!bCanReceiveDamage) return 0.f;
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("APlayerMain::TakeDamage"));
	if (Attributes && Attributes->IsAlive())
	{
		if (DamageEvent.DamageTypeClass && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
		{
			CombatComponent->GetDirectionalReact(FName("KnockDown"));
		}
		else
		{
			Attributes->ReceiveDamage(DamageAmount);
			CombatComponent->GetDirectionalReact(FName("Default"));
		}
	}
	else
	{
		Die();
	}
	return DamageAmount;
}

void APlayerMain::HitStop(float Duration, float TimeScale)
{
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(TimeScale);

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, this, &APlayerMain::ResetTimeDilation, Duration, false);
	}
}

void APlayerMain::ResetTimeDilation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(1.0f);
	}
}

AEnemy* APlayerMain::GetTargetEnemy()
{
	FVector Start;
	FVector End;

	if (FollowCamera)
	{
		Start = FollowCamera->GetActorLocation() + FollowCamera->GetActorForwardVector() * 100.0f;
		End = Start + FollowCamera->GetActorForwardVector() * PossessDistance;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_GameTraceChannel3, QueryParams))
	{
		if (AEnemy* Enemy = Cast<AEnemy>(HitResult.GetActor()))
		{
			return Enemy;
		}
		else return nullptr;
	}
	else return nullptr;
}

void APlayerMain::PossessEnemy()
{
	if (PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Spectral)
	{
		AEnemy* TargetEnemy = GetTargetEnemy();
		PlayerControllerRef = Cast<APlayerController>(GetController());

		if (PlayerControllerRef 
			&& TargetEnemy 
			&& TargetEnemy->GetEnemyState() != EEnemyState::EES_Died
			&& Attributes->RequiresEnergy(10.f)
			)
		{
			
			TargetEnemy->DisableAI();
			PlayerControllerRef->Possess(TargetEnemy);
			PossessedEnemy = TargetEnemy;

			TargetEnemy->EnableInput(PlayerControllerRef);
			TargetEnemy->AutoPossessPlayer = EAutoReceiveInput::Player0;
			TargetEnemy->OnPossessed(this);

			FollowCamera->AttachToComponent(TargetEnemy->SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
			PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);

			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			GetMesh()->bPauseAnims = true;
		}
	}
	else CombatComponent->Execute();
}

void APlayerMain::ReleasePossession()
{
	if (PlayerControllerRef)
	{
		PlayerControllerRef->Possess(this);
		PossessedEnemy->DisableInput(PlayerControllerRef);
		PossessedEnemy->EnableAI();
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorLocation(PossessedEnemy->GetActorLocation());
	SetActorRotation(PossessedEnemy->GetActorRotation());
	GetMesh()->bPauseAnims = false;
	PossessedEnemy = nullptr;
}


void APlayerMain::Move(const FInputActionValue& Value)
{
	if (CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Block || CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	const FVector2D MoveVector = Value.Get<FVector2D>();

	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(DirectionForward, MoveVector.Y);
	AddMovementInput(DirectionSideward, MoveVector.X);
}

void APlayerMain::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void APlayerMain::Jump()
{
	if (CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Block || CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	PlayAnimMontage(JumpMontage, 1.f);

	Super::Jump();

	if (GetCharacterMovement()->IsFalling() && CanDoubleJump)
	{
		DoubleJump();
	}
}

void APlayerMain::DoubleJump()
{
	if (CombatComponent->GetCharacterAction() == ECharacterActions::ECA_Block) return;

	PlayAnimMontage(DoubleJumpMontage);
	LaunchCharacter(FVector(0.f, 0.f, 800.f), false, true);
	CanDoubleJump = false;
}

void APlayerMain::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CombatComponent->bIsLaunched = false;
	CanDoubleJump = true;
}

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (InventoryComponent && InventoryComponent->IsInventoryOpen())
	{
		return;
	}

	if (PlayerFormComponent && PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		if (OverlappingItem && InventoryComponent)
		{
			const bool bAdded = InventoryComponent->TryAddItem(OverlappingItem);
			if (bAdded)
			{
				ASword* Sword = Cast<ASword>(OverlappingItem);
				if (Sword)
				{
					Sword->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);
				}
				OverlappingItem = nullptr;
			}
			// else { // Mensaje inventario lleno? }
		}
	}
	else
	{
		if (ISpectralInteractable* OverlappingObject = Cast<ISpectralInteractable>(OverlappingItem))
		{
			OverlappingObject->SpectralInteract();
		}
	}
}

void APlayerMain::Attack(const FInputActionValue& Value)
{
	CombatComponent->Input_Attack(Value);
}

void APlayerMain::HeavyAttack(const FInputActionValue& Value)
{
	CombatComponent->Input_HeavyAttack(Value);
}

void APlayerMain::LaunchAttack(const FInputActionValue& Value)
{
	CombatComponent->Input_Launch(Value);
}

void APlayerMain::Block(const FInputActionValue& Value)
{
	CombatComponent->Input_Block(Value);
}

void APlayerMain::ReleaseBlock(const FInputActionValue& Value)
{
	CombatComponent->Input_ReleaseBlock(Value);
}

void APlayerMain::Execute(const FInputActionValue& Value)
{
	CombatComponent->Input_Execute(Value);
}

void APlayerMain::ToggleForm()
{
	TArray<ECharacterActions> ActionsToCheck = { ECharacterActions::ECA_Dead, ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Attack };
	if (CombatComponent->IsActionEqualToAny(ActionsToCheck)) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastTransformationTime < TransformationCooldown) return;
	LastTransformationTime = 0;

	if (PlayerFormComponent->GetCharacterForm() != ECharacterForm::ECF_Spectral)
	{
		WithEnergy();
	}

	else
	{
		Attributes->StopDecreaseEnergy();
		OutOfEnergy();
		Attributes->RegenerateTick();
	}

	LastTransformationTime = CurrentTime;
}

void APlayerMain::WithEnergy()
{
	if (Attributes->ItHasEnergy())
	{
		PlayerFormComponent->ToggleForm(true);
		Attributes->StartDecreaseEnergy();
		Attributes->OnDepletedCallback = [this]() { OutOfEnergy(); };
		Attributes->RegenerateTick();
		GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = true;

		if (InventoryComponent->GetEquippedItem())
			InventoryComponent->GetEquippedItem()->EnableVisuals(false);
	}
}

void APlayerMain::OutOfEnergy()
{
	PlayerFormComponent->ToggleForm(false);
	Attributes->RegenerateTick();
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;
	if (InventoryComponent->GetEquippedItem()) InventoryComponent->GetEquippedItem()->EnableVisuals(true);
	if (PossessedEnemy) PossessedEnemy->UnPossess();
}

void APlayerMain::Die()
{
	if (!bIsDead)
	{
		bIsDead = true;

		if (DeathMontage)
		{
			PlayAnimMontage(DeathMontage);
		}
		
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			DisableInput(PlayerController);
		}
		
		GetCharacterMovement()->DisableMovement();
		CombatComponent->SetCharacterAction(ECharacterActions::ECA_Dead);

		FTimerHandle TimerHandle_LoadCheckpoint;
		GetWorldTimerManager().SetTimer(TimerHandle_LoadCheckpoint, this, &APlayerMain::LoadLastCheckpoint, 2.0f, false);
	}
}

void APlayerMain::Revive()
{
	if (bIsDead)
	{
		bIsDead = false;

		StopAnimMontage();

		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			EnableInput(PlayerController);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (CombatComponent)
		{
			CombatComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		}
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		CombatComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SprinEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::RestartLevel()
{	
	FName CurrentLevel = *UGameplayStatics::GetCurrentLevelName(this);
	UGameplayStatics::OpenLevel(this, CurrentLevel);	
}

void APlayerMain::GoToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}

void APlayerMain::OnWallCollision(const FHitResult& HitResult)
{
	if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("APlayerMain::OnWallCollision"));
	CombatComponent->GetDirectionalReact(FName("Default"));
}

void APlayerMain::LoadLastCheckpoint()
{
	if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
		{
			if (MementoComponent)
			{
				NewGameStateBase->LoadAllMementoStates();
				Revive();
			}
		}
	}
}

void APlayerMain::ChangePrimaryWeapon()
{
	InventoryComponent->ChangeWeapon(0);
}

void APlayerMain::ChangeSecondaryWeapon()
{
	InventoryComponent->ChangeWeapon(1);
}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerMain::Interact);

		EnhancedInputComponent->BindAction(CombatComponent->AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
		EnhancedInputComponent->BindAction(CombatComponent->HeavyAttackAction, ETriggerEvent::Triggered, this, &APlayerMain::HeavyAttack);
		EnhancedInputComponent->BindAction(CombatComponent->LaunchAction, ETriggerEvent::Triggered, this, &APlayerMain::LaunchAttack);
		EnhancedInputComponent->BindAction(CombatComponent->BlockAction, ETriggerEvent::Started, this, &APlayerMain::Block);
		EnhancedInputComponent->BindAction(CombatComponent->BlockAction, ETriggerEvent::Completed, this, &APlayerMain::ReleaseBlock);

		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Completed, this, &APlayerMain::PossessEnemy);

		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Completed, this, &APlayerMain::RestartLevel);
		EnhancedInputComponent->BindAction(GoToMenuAction, ETriggerEvent::Completed, this, &APlayerMain::GoToMainMenu);
		
		EnhancedInputComponent->BindAction(InventoryComponent->Slot1_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangePrimaryWeapon);
		EnhancedInputComponent->BindAction(InventoryComponent->Slot2_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangeSecondaryWeapon);
	}
}