#include "Enemy/Enemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/MementoComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "Player/PlayerMain.h"
#include "DamageTypes/DamageTypeMain.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "SceneEvents/NewGameModeBase.h"
#include "Tutorial/PromptWidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "SceneEvents/NewGameStateBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	GetCapsuleComponent()->SetCapsuleRadius(45.f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	SpringArm->SocketOffset = FVector(0.f,0.f,45.f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

	Memento = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));

	PromptWidgetComponent = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptWidget"));
	PromptWidgetComponent->SetupAttachment(GetRootComponent());

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;
	
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 300.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;
	GetCharacterMovement()->GravityScale = 3.f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		NewGameMode->RegisterEnemy(this);

		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
		{
			if (Memento)
			{
				NewGameStateBase->RegisterMementoEntity(this);
			}
		}
	}

	Attributes->IncreaseEnergy(FMath::RandRange(MinEnergy, MaxEnergy));
	
	if (PromptWidgetComponent)
	{
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIOriginalController = AIController;
	}

	if (GetMesh())
	{
		DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	}
}

void AEnemy::Die()
{
	SetEnemyState(EEnemyState::EES_Died);

	if (AGameStateBase* GameState = UGameplayStatics::GetGameState(GetWorld()))
	{
		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(GameState))
		{
			NewGameStateBase->UnregisterMementoEntity(this);
		}
	}

	if (PromptWidgetComponent)
	{
		PromptWidgetComponent->SetVisibility(false);
	}

	if (AGameModeBase* GameMode = UGameplayStatics::GetGameMode(GetWorld()))
	{
		if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(GameMode))
		{
			NewGameMode->UnregisterEnemy(this);
		}
	}

	ResetEnemy();

	DisableAI();

	if (APlayerMain* PlayerOwner = Cast<APlayerMain>(PossessionOwner))
	{
		if (IsValid(PlayerOwner))
		{
			UnPossess();
		}
		else
		{
			PossessionOwner = nullptr;
		}
	}
	else if (PossessionOwner != nullptr)
	{
		PossessionOwner = nullptr;
	}

	// if (APlayerMain* Player = Cast<APlayerMain>(DamageCauserOf))
	// {
	//     if (IsValid(Player) && Player->GetAttributes())
	//     {
	//         Player->GetAttributes()->IncreaseEnergy(Attributes ? Attributes->GetEnergy() : 0.f);
	//     }
	// }
	// DamageCauserOf = nullptr; // Es buena práctica limpiar punteros después de usarlos si ya no son necesarios.

	SetLifeSpan(5.f);
}

void AEnemy::NotifyThreat(AActor* ThreatActor)
{
	if (!ThreatActor)
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), ThreatActor);

		AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController);
		if (EnemyAIController)
		{
			EnemyAIController->bPauseEnemyPerceptionUpdate = true;
		}
	}
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (!EnhancedInputComponent) return;
	
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEnemy::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEnemy::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEnemy::Jump);
	EnhancedInputComponent->BindAction(UnPossessAction, ETriggerEvent::Completed, this, &AEnemy::UnPossess);
	EnhancedInputComponent->BindAction(UnPossessAndKillAction, ETriggerEvent::Completed, this, &AEnemy::UnPossessAndKill);
}

void AEnemy::Move(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(DirectionForward, MoveVector.Y);
	AddMovementInput(DirectionSideward, MoveVector.X);
}

void AEnemy::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void AEnemy::ResetEnemy()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	isLaunched = false;
	EnableAI();
}

void AEnemy::ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint)
{
	//future reactions for diff enemies
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
	if (NiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			NiagaraSystem,
			ImpactPoint
		);

		//UGameplayStatics::SpawnEmitterAtLocation(
		//	GetWorld(),
		//	HitParticles,
		//	ImpactPoint
		//);
	}
}

void AEnemy::GetFinished_Implementation()
{
	if (GetEnemyState() != EEnemyState::EES_Died)
	{
		SetEnemyState(EEnemyState::EES_Died);

		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

		FVector Start = GetActorLocation();
		FVector End = DamageCauserOf->GetActorLocation();
		FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		SetActorRotation(NewRotation);

		DisableAI();
		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->UnregisterEnemy(this);
		StopAnimMontage();
		PlayAnimMontage(FinisherDeathMontage, 1.f);
		Die();
	}
}

bool AEnemy::CanBeFinished_Implementation()
{
	return Attributes->GetHealthPercent() <= .2f;
}

bool AEnemy::IsLaunchable_Implementation()
{
	return false;
}

void AEnemy::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
}

void AEnemy::ShieldHit_Implementation()
{
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	const UDamageTypeMain* MainDamageType = DamageEvent.DamageTypeClass
		? Cast<UDamageTypeMain>(DamageEvent.DamageTypeClass->GetDefaultObject())
		: nullptr;

	LastDamageType = MainDamageType
		? MainDamageType->DamageType
		: EMainDamageTypes::EMDT_None;

	DamageCauserOf = DamageCauser;

	if (DamageCauser)
	{
		if (AEnemy* EnemyDamageCauser = Cast<AEnemy>(DamageCauser))
		{
			AAIController* AIController = Cast<AAIController>(GetController());
			if (AIController)
			{
				AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), DamageCauser);
				Cast<AEnemyAIController>(AIController)->bPauseEnemyPerceptionUpdate = true;
			}
		}

		if (Attributes)
		{
			if (Attributes->IsAlive())
			{
				Attributes->ReceiveDamage(DamageAmount);

				if (Execute_CanBeFinished(this))
				{
					PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Visible);
					PromptWidgetComponent->LoadAndApplyPrompt();
				}
			}
			else Die();
		}
	}
	return DamageAmount;
}

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();
	const FVector ToHit = (ImpactPoint - GetActorLocation()).GetSafeNormal();

	const double CosAngle = FVector::DotProduct(Forward, ToHit);

	double Angle = FMath::Acos(CosAngle);

	Angle = FMath::RadiansToDegrees(Angle);

	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0)
	{
		Angle *= -1.f;
	}

	FName Section("FromBack");

	if (Angle >= -45.f && Angle < 45.f)
	{
		Section = FName("FromFront");
	}

	else if (Angle >= -135.f && Angle < -45.f)
	{
		Section = FName("FromLeft");
	}

	else if (Angle >= 45.f && Angle < 135.f)
	{
		Section = FName("FromRight");
	}

	PlayAnimMontage(HitReactMontage, 1.f, Section);
}

void AEnemy::HitFlash()
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("HitFlashAmount"), 0.0f);
		GetWorldTimerManager().SetTimer(HitFlashTimerHandle, this, &AEnemy::ResetColor, 0.2f, false);
	}
}

void AEnemy::ResetColor()
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("HitFlashAmount"), 1.0f);
	}
}

void AEnemy::DeactivateEnemyCollision()
{
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FName AEnemy::SelectRandomDieAnim()
{
	const int32 RandomValue = FMath::RandRange(0, 3);
	FName SectionName = FName();

	switch (RandomValue)
	{
	case 0:
		return SectionName = FName("Death1");
		break;
	case 1:
		return SectionName = FName("Death2");
		break;
	case 2:
		return SectionName = FName("Death3");
		break;
	case 3:
		return SectionName = FName("Death4");
		break;
	default:
		return FName("");
		break;
	}
}

void AEnemy::SetEnemyState(EEnemyState NewState)
{
	EnemyState = NewState;
}

void AEnemy::DisableAI()
{
	if (AIOriginalController)
	{
		AIOriginalController->StopMovement();
		AIOriginalController->UnPossess();
	}
}

void AEnemy::EnableAI()
{
	if (AIOriginalController)
	{
		if (!PossessionOwner)
		{ 
			AIOriginalController->Possess(this);
			AIOriginalController->AAIController::RunBehaviorTree(BTAsset);
		}
	}
}

USpringArmComponent* AEnemy::GetSpringArm()
{
	return SpringArm;
}

void AEnemy::OnPossessed(APlayerMain* NewOwner)
{
	if (!IsValid(NewOwner)) return;

	PossessionOwner = NewOwner;

	bUseControllerRotationYaw = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
		GetCharacterMovement()->MaxWalkSpeed = 800.f;
		GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;
	}

	if (Attributes)
	{
		Attributes->StartDecreaseEnergy();
		Attributes->OnDepletedCallback = [this, WeakOwningPlayer = TWeakObjectPtr<APlayerMain>(NewOwner)]() {
			if (WeakOwningPlayer.IsValid())
			{
				UnPossess();
			}
			else
			{
				if (PossessionOwner == WeakOwningPlayer.Get(true))
				{
					PossessionOwner = nullptr;
				}
			}
			};
	}
}

void AEnemy::UnPossessBase()
{
	bUseControllerRotationYaw = true;

	if (APlayerMain* PlayerOwner = Cast<APlayerMain>(PossessionOwner))
	{
		if (IsValid(PlayerOwner))
		{
			PlayerOwner->ReleasePossession(this);
		}
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
		Cast<AEnemyAIController>(AIController)->bPauseEnemyPerceptionUpdate = false;
	}
}

void AEnemy::UnPossess()
{
	APlayerMain* PlayerOwner = Cast<APlayerMain>(PossessionOwner);

	if (PlayerOwner && IsValid(PlayerOwner))
	{
		if (PlayerOwner->GetAttributes() && PlayerOwner->GetAttributes()->RequiresEnergy(UnpossesEnergyTax))
		{
			UnPossessBase();
			PlayerOwner->GetAttributes()->DecreaseEnergyBy(UnpossesEnergyTax);
			PossessionOwner = nullptr;
			EnableAI();
		}
	}
	else PossessionOwner = nullptr;
}

void AEnemy::UnPossessAndKill()
{
	APlayerMain* PlayerToToggleForm = PossessionOwner;

	if (IsValid(PlayerToToggleForm) && PlayerToToggleForm->GetAttributes() && PlayerToToggleForm->GetAttributes()->RequiresEnergy(UnpossesAndKillEnergyTax))
	{
		UnPossessBase();

		if (IsValid(PlayerToToggleForm) && PlayerToToggleForm->GetAttributes())
		{
			PlayerToToggleForm->GetAttributes()->DecreaseEnergyBy(UnpossesAndKillEnergyTax);
		}

		PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));
		Die(); 

		if (IsValid(PlayerToToggleForm))
		{
			if (IsValid(PlayerToToggleForm->CharacterStateComponent))
			{
				PlayerToToggleForm->ToggleForm();
			}
		}

		EnableAI();
		PossessionOwner = nullptr;
	}
}
