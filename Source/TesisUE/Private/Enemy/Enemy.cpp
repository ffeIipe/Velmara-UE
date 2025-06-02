#include "Enemy/Enemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/MementoComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/TimelineComponent.h"
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
#include "BehaviorTree/BehaviorTree.h"
#include "Subsystems/EnemyPoolManager.h"
#include <Enemy/Paladin/PaladinBoss.h>
#include "Misc/Guid.h"
#include "Materials/MaterialParameterCollectionInstance.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	if (UniqueSaveID == NAME_None)
	{
		UniqueSaveID = FName(*FGuid::NewGuid().ToString());
	}

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	InitialMeshCollisionEnabled = GetMesh()->GetCollisionEnabled();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	GetCapsuleComponent()->SetCapsuleRadius(45.f);
	InitialCapsuleCollisionEnabled = GetCapsuleComponent()->GetCollisionEnabled();

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	SpringArm->SocketOffset = FVector(0.f, 0.f, 45.f);
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

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterStateComponent"));

	bInitialMeshGenerateOverlapEvents = GetMesh()->GetGenerateOverlapEvents();
	bInitialCapsuleGenerateOverlapEvents = GetCapsuleComponent()->GetGenerateOverlapEvents();

	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	DefaultGravityScale = GetCharacterMovement()->GravityScale;
	DefaultJumpZVelocity = GetCharacterMovement()->JumpZVelocity;
	bDefaultOrientRotationToMovement = GetCharacterMovement()->bOrientRotationToMovement;
	bDefaultUseControllerDesiredRotation = GetCharacterMovement()->bUseControllerDesiredRotation;
	bOriginalUseControllerRotationYaw = bUseControllerRotationYaw;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	DissolveParticleComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Dissolve"));
	DissolveParticleComponent->SetupAttachment(GetMesh());

	ExtraMovementComponent = CreateDefaultSubobject<UExtraMovementComponent>(TEXT("ExtraMovementComponent"));
}

void AEnemy::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	EnemyState = EEnemyState::EES_None;
	isLaunched = false;
	DamageCauserOf = nullptr;
	bWasPossessed = false;

	if (Attributes)
	{
		Attributes->ResetAttributes();
	}

	if (CharacterStateComponent)
	{
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
	}

	bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
		GetCharacterMovement()->GravityScale = DefaultGravityScale;
		GetCharacterMovement()->JumpZVelocity = DefaultJumpZVelocity;
		GetCharacterMovement()->bOrientRotationToMovement = bDefaultOrientRotationToMovement;
		GetCharacterMovement()->bUseControllerDesiredRotation = bDefaultUseControllerDesiredRotation;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 300.f, 0.f);
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}

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

	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	GetMesh()->SetSimulatePhysics(false);

	EnableAI(); 

	DissolveTimeline->Reverse();
}

void AEnemy::DeactivateEnemy()
{
	if (PossessionOwner)
	{
		APlayerMain* PlayerOwnerRef = Cast<APlayerMain>(PossessionOwner);
		if (PlayerOwnerRef && IsValid(PlayerOwnerRef))
		{
			UnPossessBase();
		}
		PossessionOwner = nullptr;
	}

	DisableAI();

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	DeactivateEnemyCollision();

	StopAnimMontage();

	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->SetVisibility(false);
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	AAIController* AIControllerInstance = Cast<AAIController>(GetController());
	if (!AIControllerInstance && AIOriginalController)
	{
		AIControllerInstance = AIOriginalController;
	}
	if (AIControllerInstance && AIControllerInstance->GetBlackboardComponent())
	{
		AIControllerInstance->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
		if (AEnemyAIController* EnemyAICont = Cast<AEnemyAIController>(AIControllerInstance))
		{
			EnemyAICont->bPauseEnemyPerceptionUpdate = false;
		}
	}

	EnemyState = EEnemyState::EES_None;
	isLaunched = false;
	DamageCauserOf = nullptr;

	OnDeactivated.Broadcast(this);

	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToPoolTimerHandle);
}

void AEnemy::Die(AActor* DamageCauser)
{
	SetEnemyState(EEnemyState::EES_Died);
	if (CharacterStateComponent) CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Dead);

	DamageCauserOf = DamageCauser;

	if (UWorld* World = GetWorld())
	{
		if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
		{
			FEnemySaveData CurrentStateData;
			CurrentStateData.UniqueSaveID = GetUniqueSaveID();
			CurrentStateData.bIsAlive = false;
			CurrentStateData.EnemyClass = GetClass();

			if (Memento)
			{
				CurrentStateData.EnemyState = Memento->CaptureOwnerState();
			}

			GameState->UpdateEnemyState(CurrentStateData);
		}
	}

	APlayerMain* PlayerCharacter = Cast<APlayerMain>(PossessionOwner);
	if (PlayerCharacter && IsValid(PlayerCharacter))
	{
		// UnPossess() handles the logic for player unpossessing this enemy.
		// UnPossessAndKill calls UnPossessBase. If Die() is called by something else while possessed,
		// we might need to ensure a clean unpossession.
		// The DeactivateEnemy() has a robust check now.
	}
	else if (PossessionOwner != nullptr)
	{
		PossessionOwner = nullptr;
	}

	if (DamageCauser && DamageCauser->GetComponentByClass<UAttributeComponent>())
	{
		DamageCauser->GetComponentByClass<UAttributeComponent>()->IncreaseEnergy(FMath::RandRange(MinEnergy, MaxEnergy));
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, FString("Increasing energy..."));

	}
	
	DisableAI();

	GetWorldTimerManager().SetTimer(ReturnToPoolTimerHandle, this, &AEnemy::RequestReturnToPool, 5.0f, false);
}

void AEnemy::PoolableDie(AActor* DamageCauser)
{
	Die(DamageCauser);
}

void AEnemy::RequestReturnToPool()
{
	DeactivateEnemy();

	UWorld* World = GetWorld();
	if (World)
	{
		UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>();
		if (PoolManager)
		{
			PoolManager->ReturnEnemyToPool(this);
		}
		else
		{
			if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::White, FString("Not valid Pool Subsystem"));
			Destroy();
		}
	}
	else
	{
		Destroy();
	}
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (DissolveCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("UpdateDissolveEffect"));
		DissolveTimeline->AddInterpFloat(DissolveCurve, ProgressFunction);
	}

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

	if (UWorld* World = GetWorld())
	{
		ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>();
		if (GameState)
		{
			GameState->RequestEnemyStateReconciliation(this);
		}
	}
	
	if (IsValid(PromptWidgetComponent->GetWidget()))
	{
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	AAIController* AIControllerInstance = Cast<AAIController>(GetController());
	if (AIControllerInstance)
	{
		AIOriginalController = AIControllerInstance;
	}

	if (GetMesh())
	{
		DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	}
}

void AEnemy::Jump()
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }))
	{
		if (CharacterStateComponent->IsFormEqualToAny({ ECharacterForm::ECF_Spectral }) &&
			CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Dodge })) return;

		PlayAnimMontage(ExtraMovementComponent->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && ExtraMovementComponent->CanDoubleJump)
		{
			ExtraMovementComponent->Input_DoubleJump();
		}
	}
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

void AEnemy::UpdateDissolveEffect(float Value)
{
	if (DynamicMaterial && DissolveParticleComponent)
	{
		float ClampedValue = FMath::Clamp(Value, 0.f, 1.f);
		DynamicMaterial->SetScalarParameterValue(FName("Animation"), ClampedValue);

		DissolveParticleComponent->SetNiagaraVariableFloat(FString("User_Animation"), ClampedValue);

		GEngine->AddOnScreenDebugMessage(1, -1.f, FColor::Cyan, FString::SanitizeFloat(ClampedValue));
	}
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

void AEnemy::DoubleJump(const FInputActionValue& Value)
{
	ExtraMovementComponent->Input_DoubleJump();
}

void AEnemy::Dodge(const FInputActionValue& Value)
{
	ExtraMovementComponent->Input_Dodge();
}

void AEnemy::ResetEnemy()
{
	isLaunched = false;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	EnableAI();
}

void AEnemy::ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint)
{
	//future reactions for diff enemies
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType)
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
	}
}

void AEnemy::GetFinished_Implementation()
{
	if (GetEnemyState() != EEnemyState::EES_Died)
	{
		SetEnemyState(EEnemyState::EES_Died);
		if (CharacterStateComponent) CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Dead);

		if (PromptWidgetComponent && PromptWidgetComponent->GetWidget()) PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

		FVector Start = GetActorLocation();
		FVector End = DamageCauserOf ? DamageCauserOf->GetActorLocation() : Start + GetActorForwardVector();
		FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		SetActorRotation(NewRotation);

		DisableAI();
		StopAnimMontage();
		PlayAnimMontage(FinisherDeathMontage, 1.f); 

		Die(DamageCauserOf);
	}
}

bool AEnemy::CanBeFinished_Implementation()
{
	return Attributes->GetHealthPercent() <= .2f;
}

bool AEnemy::IsLaunchable_Implementation(ACharacter* Character)
{
	return false;
}

void AEnemy::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
}

void AEnemy::ShieldHit_Implementation()
{
}

UMementoComponent* AEnemy::GetMementoComponent_Implementation()
{
	return Memento;
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (EnemyState == EEnemyState::EES_Died)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

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
			if (APaladinBoss* PaladinBoss = Cast<APaladinBoss>(EnemyDamageCauser)) return 0.0f;

			AAIController* AIController = Cast<AAIController>(GetController());
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), DamageCauser);
				if (AEnemyAIController* EnemyAICont = Cast<AEnemyAIController>(AIController)) EnemyAICont->bPauseEnemyPerceptionUpdate = true;
			}
		}
	}

	if (Attributes)
	{
		if (Attributes->IsAlive())
		{
			Attributes->ReceiveDamage(ActualDamage);

			if (Execute_CanBeFinished(this))
			{
				if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
				{
					PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Visible);
					PromptWidgetComponent->LoadAndApplyPrompt();
				}
			}
		}
	}
	return ActualDamage;
}

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage)
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

	PlayAnimMontage(HitReactAnimMontage, 1.f, Section);
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
	else
	{
		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			AIController->StopMovement();
			AIController->UnPossess();
		}
	}
}

void AEnemy::EnableAI()
{
	if (!AIOriginalController)
	{
		AAIController* CurrentController = Cast<AAIController>(GetController());
		if (CurrentController)
		{
			AIOriginalController = CurrentController;
		}
		else return;
	}

	if (AIOriginalController)
	{
		if (!PossessionOwner)
		{
			AIOriginalController->Possess(this);

			if (AIOriginalController->GetPawn() == this)
			{
				if (BTAsset)
				{
					bool bRunResult = AIOriginalController->RunBehaviorTree(BTAsset);
				}
			}
		}
	}
}

USpringArmComponent* AEnemy::GetSpringArm()
{
	return SpringArm;
}

void AEnemy::OnPossessed(APlayerMain* NewOwner, float OwnerEnergy)
{
	if (!IsValid(NewOwner)) return;

	PossessionOwner = NewOwner;

	Attributes->SetEnergy(OwnerEnergy);

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
			PlayerOwner->GetAttributes()->SetEnergy(Attributes->GetEnergy());
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

		DisableAI();
		StopAnimMontage();
		PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));

		Die(DamageCauserOf);

		if (IsValid(PlayerToToggleForm))
		{
			if (IsValid(PlayerToToggleForm->CharacterStateComponent))
			{
				PlayerToToggleForm->ToggleForm();
			}
		}
		PossessionOwner = nullptr;
	}
	else if (Attributes && !Attributes->RequiresEnergy(UnpossesAndKillEnergyTax) && ErrorSFX)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ErrorSFX);
	}
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEnemy::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEnemy::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEnemy::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEnemy::DoubleJump);
	EnhancedInputComponent->BindAction(ExtraMovementComponent->DodgeAction, ETriggerEvent::Triggered, this, &AEnemy::Dodge);
	EnhancedInputComponent->BindAction(UnPossessAction, ETriggerEvent::Completed, this, &AEnemy::UnPossess);
	EnhancedInputComponent->BindAction(UnPossessAndKillAction, ETriggerEvent::Completed, this, &AEnemy::UnPossessAndKill);
}