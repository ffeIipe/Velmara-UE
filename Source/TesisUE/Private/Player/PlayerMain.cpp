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

	Memento = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
}

ECharacterForm APlayerMain::GetCharacterForm_Implementation()
{
	return PlayerFormComponent->GetCharacterForm();
}

void APlayerMain::PerformSpectralAttack_Implementation()
{
	if (Combat->GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		Combat->SetCharacterAction(ECharacterActions::ECA_Attack);
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
	if (Combat->GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		Combat->SetCharacterAction(ECharacterActions::ECA_Attack);
		PlayAnimMontage(SpectralHeavyAttack);
	}
}

void APlayerMain::ResetSpectralAttack_Implementation()
{
	SpectralAttackIndex = 0;
	Combat->bIsSaveLightAttack = false;
}

void APlayerMain::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (EquippedWeapon && EquippedWeapon->GetWeaponBox())
	{
		EquippedWeapon->GetWeaponBox()->SetCollisionEnabled(CollisionEnabled);
		EquippedWeapon->IgnoreActors.Empty();
	}
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();

	PlayerControllerRef = Cast<APlayerController>(GetController());

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
			if (Memento)
			{
				NewGameStateBase->RegisterMementoEntity(this);
			}
		}
	}
}

void APlayerMain::PerformDodge()
{
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	FVector MovementInput = GetLastMovementInputVector();
	if (!MovementInput.IsNearlyZero())
	{
		FRotator LookRotation = MovementInput.Rotation();
		SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
	}

	StopDodgeBufferEvent();
	DodgeBufferEvent(BufferDodgeDistance);
	Combat->SetCharacterAction(ECharacterActions::ECA_Dodge);

	if (PlayerFormComponent && PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		PlayAnimMontage(DodgeMontage);
	}
	else
	{
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

	AActor* Enemy = Combat->SphereTraceForEnemies(Start, End);

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
	
	if (Attributes && Attributes->IsAlive())
	{
		Attributes->ReceiveDamage(DamageAmount);
		Combat->GetDirectionalReact();
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
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);
		return Cast<AEnemy>(HitResult.GetActor());
	}
	return nullptr;
}

void APlayerMain::PossessEnemy()
{
	if (PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Spectral)
	{
		AEnemy* TargetEnemy = GetTargetEnemy();
		if (!TargetEnemy && !Attributes->RequiresEnergy(10.f)) return;

		PlayerControllerRef = Cast<APlayerController>(GetController());

		if (PlayerControllerRef)
		{
			PlayerControllerRef->Possess(TargetEnemy);
			TargetEnemy->DisableAI();
			PossessedEnemy = TargetEnemy;

			TargetEnemy->EnableInput(PlayerControllerRef);
			TargetEnemy->AutoPossessPlayer = EAutoReceiveInput::Player0;
			TargetEnemy->OnPossessed(this);

			FollowCamera->AttachToComponent(TargetEnemy->SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
			PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
		}
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
	else Combat->FinishEnemy();
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
	PossessedEnemy = nullptr;
}


void APlayerMain::Move(const FInputActionValue& Value)
{
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Block || Combat->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

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
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void APlayerMain::Jump()
{
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Block || Combat->GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	PlayAnimMontage(JumpMontage, 1.f);

	Super::Jump();

	if (GetCharacterMovement()->IsFalling() && CanDoubleJump)
	{
		DoubleJump();
	}
}

void APlayerMain::DoubleJump()
{
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Block) return;

	PlayAnimMontage(DoubleJumpMontage);
	LaunchCharacter(FVector(0.f, 0.f, 800.f), false, true);
	CanDoubleJump = false;
}

void APlayerMain::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	Combat->bIsLaunched = false;
	CanDoubleJump = true;
}

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		if (ASword* OverlappingWeapon = Cast<ASword>(OverlappingItem))
		{
			OverlappingWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
			Combat->SetCharacterState(ECharacterStates::ECS_EquippedSword);
			OverlappingItem = nullptr;
			EquippedWeapon = OverlappingWeapon;
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
	Combat->Input_Attack(Value);
}

void APlayerMain::HeavyAttack(const FInputActionValue& Value)
{
	Combat->Input_HeavyAttack(Value);
}

void APlayerMain::LaunchAttack(const FInputActionValue& Value)
{
	Combat->Input_Launch(Value);
}

void APlayerMain::ToggleForm() //TODO: extract to PlayerMain the "apply effects" functions for more flexibility here
{
	if (Combat->GetCharacterAction() == ECharacterActions::ECA_Dead
		|| Combat->GetCharacterAction() == ECharacterActions::ECA_Block
		|| Combat->GetCharacterAction() == ECharacterActions::ECA_Finish
		|| Combat->GetCharacterAction() == ECharacterActions::ECA_Attack) return;

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

void APlayerMain::Block()
{
	
}

void APlayerMain::ReleaseBlock()
{

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

		if (EquippedWeapon)
			EquippedWeapon->Enable(false);		
	}
}

void APlayerMain::OutOfEnergy()
{
	PlayerFormComponent->ToggleForm(false);
	Attributes->RegenerateTick();
	GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;
	if (EquippedWeapon) EquippedWeapon->Enable(true);
	if (PossessedEnemy) PossessedEnemy->UnPossess();
}

void APlayerMain::Die()
{
	if (!bIsDead)
	{
		bIsDead = true;

		//if (DeathMontage)
		//{
		//	PlayAnimMontage(DeathMontage);
		//}
		//
		//APlayerController* PlayerController = Cast<APlayerController>(GetController());
		//if (PlayerController)
		//{
		//	DisableInput(PlayerController);
		//}
		//
		//GetCharacterMovement()->DisableMovement();
		//SetCharacterAction(ECharacterActions::ECA_Dead);

		if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
			{
				if (Memento)
				{
					NewGameStateBase->LoadAllMementoStates();
				}
			}
		}
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		Combat->SetCharacterAction(ECharacterActions::ECA_Nothing);
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

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerMain::Interact);
		EnhancedInputComponent->BindAction(Combat->AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
		EnhancedInputComponent->BindAction(Combat->HeavyAttackAction, ETriggerEvent::Triggered, this, &APlayerMain::HeavyAttack);
		EnhancedInputComponent->BindAction(Combat->LaunchAction, ETriggerEvent::Triggered, this, &APlayerMain::LaunchAttack);
		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(Combat->BlockAction, ETriggerEvent::Started, this, &APlayerMain::Block);
		EnhancedInputComponent->BindAction(Combat->BlockAction, ETriggerEvent::Completed, this, &APlayerMain::ReleaseBlock);
		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Completed, this, &APlayerMain::PossessEnemy);
		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Completed, this, &APlayerMain::RestartLevel);
		EnhancedInputComponent->BindAction(GoToMenuAction, ETriggerEvent::Completed, this, &APlayerMain::GoToMainMenu);
	}
}