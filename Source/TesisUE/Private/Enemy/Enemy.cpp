#include "Enemy/Enemy.h"
#include "AI/EnemyAIController.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/MementoComponent.h"
#include "Components/TimelineComponent.h"
#include "DamageTypes/DamageTypeMain.h"
#include "Engine/DamageEvents.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "HUD/HealthBarComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/Guid.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/PlayerMain.h"
#include "SceneEvents/NewGameModeBase.h"
#include "SceneEvents/NewGameStateBase.h"
#include "Subsystems/EnemyPoolManager.h"
#include "Tutorial/PromptWidgetComponent.h"
#include "Player/PlayerHeroController.h"
#include "HUD/PlayerMainHUD.h"
AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	UniqueSaveID = FName(*FGuid::NewGuid().ToString());
	

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

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));

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

	bIsPooledInstance = false;
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

	GetMesh()->bPauseAnims = false;

	HandleEnemyCollision(ECR_Block);

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

	GetDefaultParameters();

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
	HandleEnemyCollision(ECR_Ignore);

	StopAnimMontage();
	GetMesh()->bPauseAnims = true;

	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->SetVisibility(false);
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}

	AAIController* AIControllerInstance = Cast<AAIController>(GetController());
	if (!AIControllerInstance && AIController)
	{
		AIControllerInstance = AIController;
	}
	if (AIControllerInstance && AIControllerInstance->GetBlackboardComponent())
	{
		AIControllerInstance->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
		AIControllerInstance->GetBlackboardComponent()->ClearValue(FName("CanSeePlayer"));
	}

	isLaunched = false;
	DamageCauserOf = nullptr;

	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToPoolTimerHandle);
}

void AEnemy::Die(AActor* DamageCauser)
{
	if (EnemyState == EEnemyState::EES_Died) return;

	SetEnemyState(EEnemyState::EES_Died);

	StopAnimMontage();
	PlayAnimMontage(DeathMontage, 1.f, SelectRandomDieAnim());

	DissolveTimeline->PlayFromStart();

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

	if (APlayerMain* PlayerRef = Cast<APlayerMain>(DamageCauser))
	{
		PlayerRef->Attributes->IncreaseEnergy(FMath::RandRange(MinEnergy, MaxEnergy));
	}

	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->GetPromptWidgetComponent()->EnablePromptWidget(false);
	}

	DisableAI();
	HandleEnemyCollision(ECR_Ignore);

	if (PossessionOwner)
	{
		UnPossess();
	}

	OnDeactivated.Broadcast(this);

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

	PlayerControllerRef = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(this, 0));

	GetDefaultParameters();

	CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword);

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
		AIController = AIControllerInstance;
		EnemyAIController = Cast<AEnemyAIController>(AIController);
	}

	if (GetMesh())
	{
		for (int32 i = 0; i < GetMesh()->GetMaterials().Num(); ++i)
		{
			DissolveMaterials.Add(UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(i), this));
			GetMesh()->SetMaterial(i, DissolveMaterials[i]);
		}
	}

	ActivateEnemy(GetActorLocation(),GetActorRotation());
}

void AEnemy::NotifyThreat(AActor* ThreatActor)
{
	if (!ThreatActor)
	{
		return;
	}

	if (EnemyAIController)
	{
		EnemyAIController->DamageCauser = ThreatActor;
	}

	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), ThreatActor);
		AIController->GetBlackboardComponent()->SetValueAsBool(FName("CanSeePlayer"), true);
	}

	// una func con un forget?
}

void AEnemy::UpdateDissolveEffect(float Value)
{
	float ClampedValue = FMath::Clamp(Value, 0.f, 1.f);

	for (UMaterialInstanceDynamic* DissolveMaterial : DissolveMaterials)
	{
		if (IsValid(DissolveMaterial))
		{
			DissolveMaterial->SetScalarParameterValue(FName("Animation"), ClampedValue);
		}
	}

	if (IsValid(DissolveParticleComponent))
	{
		DissolveParticleComponent->SetNiagaraVariableFloat(FString("User_Animation"), ClampedValue);
	}
}

void AEnemy::Move(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun }))
	{
		const FVector2D MoveVector = Value.Get<FVector2D>();

		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(DirectionForward, MoveVector.Y);
		AddMovementInput(DirectionSideward, MoveVector.X);
	}
}

void AEnemy::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void AEnemy::Attack(const FInputActionValue& Value)
{
	//sus children se encargan de overridear esta funcion
}

void AEnemy::Jump()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Nothing, ECharacterActions::ECA_Attack }) && ExtraMovementComponent->CanDoubleJump)
	{
		PlayAnimMontage(ExtraMovementComponent->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && ExtraMovementComponent->CanDoubleJump)
		{
			ExtraMovementComponent->Input_DoubleJump();
		}
	}
}

void AEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CombatComponent->bIsLaunched = false;
	ExtraMovementComponent->CanDoubleJump = true;
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
	
}

void AEnemy::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	if (Attributes->IsAlive())
	{
		ReactToDamage(LastDamageType, ImpactPoint);
		DirectionalHitReact(ImpactPoint, HitReactMontage, DamageReceived);

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

		if (APlayerMain* PlayerRef = Cast<APlayerMain>(DamageCauser))
		{
			float Percentage = DamageReceived / EnergyDivider;
			PlayerRef->Attributes->IncreaseEnergy(Percentage);
		}
	}
}

void AEnemy::GetFinished_Implementation()
{
	if (GetEnemyState() != EEnemyState::EES_Died)
	{
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

UCharacterStateComponent* AEnemy::GetCharacterStateComponent_Implementation()
{
	return CharacterStateComponent;
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (EnemyState == EEnemyState::EES_Died)
	{
		return 0.f;
	}

	//limpiar esta garompa
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	const UDamageTypeMain* MainDamageType = DamageEvent.DamageTypeClass
		? Cast<UDamageTypeMain>(DamageEvent.DamageTypeClass->GetDefaultObject())
		: nullptr;

	LastDamageType = MainDamageType
		? MainDamageType->DamageType
		: EMainDamageTypes::EMDT_None;

	DamageCauserOf = DamageCauser;

	if (Attributes)
	{
		if (Attributes->IsAlive())
		{
			Attributes->ReceiveDamage(ActualDamage);
			NotifyDamageTakenToBlackboard(DamageCauser);

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

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage, const float DamageReceived)
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

	if (DamageReceived > DamageThreshold)
	{
		Section = FName("FromBackBig");
	}
	
	if (Angle >= -45.f && Angle < 45.f)
	{
		if (DamageReceived > DamageThreshold)
		{
			Section = FName("FromFrontBig");
		}
		else
		{
			Section = FName("FromFront");
		}	
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

void AEnemy::GetDefaultParameters()
{
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
}

void AEnemy::SetOnPossessedParameters()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
		GetCharacterMovement()->MaxWalkSpeed = 800.f;
		GetCharacterMovement()->BrakingDecelerationWalking = 1000.f; 
	}
}

void AEnemy::HandleEnemyCollision(ECollisionResponse CollisionResponse)
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, CollisionResponse);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, CollisionResponse); //sword trace
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, CollisionResponse); //spectral weapon trace
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, CollisionResponse);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, CollisionResponse);
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
	if (AIController)
	{
		AIController->StopMovement();
		AIController->UnPossess();
	}
}

void AEnemy::EnableAI()
{
	if (!AIController)
	{
		AAIController* CurrentController = Cast<AAIController>(GetController());
		if (CurrentController)
		{
			AIController = CurrentController;
		}
		else return;
	}

	if (AIController)
	{
		if (!PossessionOwner)
		{
			AIController->Possess(this);

			if (AIController->GetPawn() == this)
			{
				if (BTAsset)
				{
					bool bRunResult = AIController->RunBehaviorTree(BTAsset);
				}
			}
		}
	}
}

void AEnemy::OnPossessed(APlayerMain* NewOwner, float OwnerEnergy)
{
	if (!IsValid(NewOwner)) return;

	PossessionOwner = NewOwner;

	Attributes->SetEnergy(OwnerEnergy);

	bUseControllerRotationYaw = false;
	
	SetOnPossessedParameters();

	PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(true);

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
					UnPossess();
					PossessionOwner = nullptr;
				}
			}
			};
	}
}

void AEnemy::UnPossessBase()
{
	bUseControllerRotationYaw = true;

	GetDefaultParameters(); // gettear y settear parametros por defecto... se puede mejorar con algo mas lindo como un struct que gettee y settee... 

	PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(false);

	if (PossessionOwner)
	{
		if (IsValid(PossessionOwner))
		{
			PossessionOwner->ReleasePossession(this);
		}
	}

	if (AIController)
	{
		AIController->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
		AIController->GetBlackboardComponent()->ClearValue(FName("CanSeePlayer"));	
	}
}

void AEnemy::UnPossess()
{
	if (PossessionOwner)
	{
		if (PossessionOwner->GetAttributes())
		{
			UnPossessBase();
			PossessionOwner->GetAttributes()->SetEnergy(Attributes->GetEnergy());
			PossessionOwner = nullptr;
			EnableAI();
		}
	}
	else PossessionOwner = nullptr;
}

void AEnemy::UnPossessAndKill()
{
	if (PossessionOwner && PossessionOwner->GetAttributes() && PossessionOwner->GetAttributes()->RequiresEnergy(UnpossesAndKillEnergyTax))
	{
		float NewPercentage = (Attributes->GetHealthPercent() / 2) * 100.f;
		PossessionOwner->Attributes->SetHealth(Attributes->GetHealth() + NewPercentage);

		UnPossessBase();

		if (IsValid(PossessionOwner) && PossessionOwner->GetAttributes())
		{
			PossessionOwner->GetAttributes()->DecreaseEnergyBy(UnpossesAndKillEnergyTax);
		}

		DisableAI();
		StopAnimMontage();
		PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));

		Die(DamageCauserOf);

		if (IsValid(PossessionOwner))
		{
			if (IsValid(PossessionOwner->CharacterStateComponent))
			{
				PossessionOwner->ToggleForm();
			}
		}
		PossessionOwner = nullptr;
	}
	else if (Attributes && !Attributes->RequiresEnergy(UnpossesAndKillEnergyTax) && ErrorSFX)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ErrorSFX);
	}
}

TArray<AEnemy*> AEnemy::GenerateSphereOverlapToDetectOtherEnemies(const FVector& Origin, AActor* HitEnemyToExclude)
{
	TArray<AActor*> ActorsToIgnoreForSphere;
	ActorsToIgnoreForSphere.Add(this);
	if (HitEnemyToExclude)
	{
		ActorsToIgnoreForSphere.Add(HitEnemyToExclude);
	}

	TArray<AActor*> OverlappedActors;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	bool bOverlapOccurred = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Origin,
		RadiusToNotifyAllies,
		ObjectTypes,
		AEnemy::StaticClass(),
		ActorsToIgnoreForSphere,
		OverlappedActors
	);

	DrawDebugSphere(
		GetWorld(),
		Origin,
		RadiusToNotifyAllies,
		24,
		FColor::Yellow,
		false,
		5.0f
	);

	TArray<AEnemy*> EnemiesFound;
	if (bOverlapOccurred)
	{
		for (AActor* Actor : OverlappedActors)
		{
			AEnemy* EnemyActor = Cast<AEnemy>(Actor);

			if (EnemyActor)
			{
				EnemiesFound.Add(EnemyActor);
			}
		}
	}

	return EnemiesFound;
}

void AEnemy::NotifyDamageTakenToBlackboard(AActor* DamageCauser)
{
	AEnemy* EnemyRef = Cast<AEnemy>(DamageCauser);
	APlayerMain* PlayerRef = Cast<APlayerMain>(DamageCauser);
	
	if (EnemyRef && EnemyRef->GetPossessionOwner() || PlayerRef)
	{
		if (AIController)
		{
			AIController->GetBlackboardComponent()->SetValueAsBool(FName("DamageTakenRecently"), true);
			AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), DamageCauser);
			AIController->GetBlackboardComponent()->SetValueAsBool(FName("CanSeePlayer"), true);
		}

		if (EnemyAIController)
		{
			EnemyAIController->DamageCauser = DamageCauser;
		}

		for (AEnemy* Enemy : GenerateSphereOverlapToDetectOtherEnemies(GetActorLocation(), this))
		{
			Enemy->NotifyThreat(DamageCauser);
		}
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
	EnhancedInputComponent->BindAction(ExtraMovementComponent->DodgeAction, ETriggerEvent::Triggered, this, &AEnemy::Dodge);

	EnhancedInputComponent->BindAction(CombatComponent->AttackAction, ETriggerEvent::Triggered, this, &AEnemy::Attack);

	EnhancedInputComponent->BindAction(UnPossessAction, ETriggerEvent::Completed, this, &AEnemy::UnPossess);
	EnhancedInputComponent->BindAction(UnPossessAndKillAction, ETriggerEvent::Completed, this, &AEnemy::UnPossessAndKill);
}