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
#include "Components/PossessionComponent.h"
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
	
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	InitialMeshCollisionEnabled = GetMesh()->GetCollisionEnabled();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	GetCapsuleComponent()->SetCapsuleRadius(45.f);
	InitialCapsuleCollisionEnabled = GetCapsuleComponent()->GetCollisionEnabled();

	PromptWidgetComponent = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptWidget"));
	PromptWidgetComponent->SetupAttachment(GetRootComponent());

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 300.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;
	GetCharacterMovement()->GravityScale = 3.f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;

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
}

void AEnemy::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);
	SetActorHiddenInGame(false);
	//SetActorTickEnabled(true);

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
	LastDamageCauser = nullptr;
	bWasPossessed = false;

	if (GetAttributeComponent())
	{
		GetAttributeComponent()->ResetAttributes();
	}

	if (GetCharacterStateComponent())
	{
		GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Nothing);
	}

	bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;

	GetDefaultParameters();

	if (ANewGameModeBase* NewGameMode = Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		NewGameMode->RegisterEnemy(this);

		if (ANewGameStateBase* NewGameStateBase = Cast<ANewGameStateBase>(NewGameMode->GameState))
		{
			if (GetMementoComponent())
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

	DisableAI();

	isLaunched = false;
	LastDamageCauser = nullptr;
	
	if (OnDeactivated.IsBound())
	{
		OnDeactivated.Broadcast(this);
	}
	
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToPoolTimerHandle);
}

void AEnemy::Die()
{
	Super::Die();

	DissolveTimeline->PlayFromStart();

	//save data de que murio
	if (UWorld* World = GetWorld())
	{
		if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
		{
			FEnemySaveData CurrentStateData;
			CurrentStateData.UniqueSaveID = GetUniqueSaveID();
			CurrentStateData.bIsAlive = false;
			CurrentStateData.EnemyClass = GetClass();

			if (GetMementoComponent())
			{
				CurrentStateData.EnemyState = GetMementoComponent()->CaptureOwnerState();
			}

			GameState->UpdateEnemyState(CurrentStateData);
		}
	} 

	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())//enemy
	{
		PromptWidgetComponent->GetPromptWidgetComponent()->EnablePromptWidget(false);
	}

	DisableAI();
	HandleEnemyCollision(ECR_Ignore);

	if (GetPossessionComponent()->GetPossessedEntity())
	{
		GetPossessionComponent()->ReleasePossession();
	}

	if (OnDeactivated.IsBound())
	{
		OnDeactivated.Broadcast(this);
	}

	GetWorldTimerManager().SetTimer(ReturnToPoolTimerHandle, this, &AEnemy::RequestReturnToPool, 5.0f, false);
}

void AEnemy::PoolableDie()
{
	Die();
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

	OnCanBeFinished.AddDynamic(this, &AEnemy::EnableFinisherWidget);
	
	GetPossessionComponent()->OnPossessorEjected.AddDynamic(this, &AEnemy::EnableAI);
	GetPossessionComponent()->OnPossessorExecutedMeAndEjected.AddDynamic(this, &AEnemy::GetExecuted);

	PlayerControllerRef = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(this, 0));

	HandleEnemyCollision(ECR_Block);

	GetDefaultParameters();

	if (GetCharacterStateComponent())
	{
		GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippedSword);
	}

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
			if (GetMementoComponent())
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

	if (AAIController* AIControllerInstance = Cast<AAIController>(GetController()))
	{
		AIController = AIControllerInstance;
		EnemyAIController = Cast<AEnemyAIController>(AIController);
	}

	if (UBlackboardComponent* BBComponentInstance = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent()))
	{
		BBComponent = BBComponentInstance;
	}
	/*else GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Cast to BBComponent failed..."));*/

	if (GetMesh())
	{
		TArray<UMaterialInterface*> TempArray = GetMesh()->GetMaterials();

		for (int32 MatIndex = 0; MatIndex < TempArray.Num(); MatIndex++)
		{
			UMaterialInterface* CurrentMaterial = GetMesh()->GetMaterial(MatIndex);
			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);

			DissolveMaterials.Add(UMaterialInstanceDynamic::Create(CurrentMaterial, this));
			GetMesh()->SetMaterial(MatIndex, DissolveMaterials[MatIndex]);
		}
	}
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputAction_Possess, ETriggerEvent::Started, GetPossessionComponent(), &UPossessionComponent::EjectPossessor);
		EnhancedInputComponent->BindAction(InputAction_SwitchForm, ETriggerEvent::Started, GetPossessionComponent(), &UPossessionComponent::EjectAndExecute);
	}
}

void AEnemy::EnableFinisherWidget()
{
	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Visible);
		PromptWidgetComponent->LoadAndApplyPrompt();
	}
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

void AEnemy::ResetEnemy()
{
	isLaunched = false;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	EnableAI();
}

void AEnemy::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived)
{
	Super::GetHit_Implementation(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);
	
	if (DamageEvent.DamageTypeClass)
	{
		if (UDamageTypeMain* MainDamageTypeClass = Cast<UDamageTypeMain>(DamageEvent.DamageTypeClass->GetDefaultObject()))
		{
			EMainDamageTypes MainDamageType = MainDamageTypeClass->DamageType;
			ReactToDamage(MainDamageType, ImpactPoint);
		}
	}

	DropOrbs(DamageReceived, DamageCauser);
	NotifyDamageTakenToBlackboard(DamageCauser);
}

void AEnemy::DropOrbs(const float DamageReceived, AActor* DamageCauser)
{
	float Percentage = DamageReceived / EnergyDivider;
	int32 Orbs = FMath::RoundToInt(Percentage) / 5;

	if (APlayerMain* PlayerRef = Cast<APlayerMain>(DamageCauser))
	{
		PlayerRef->GetAttributeComponent()->IncreaseEnergy(Percentage);

		if (OnDamaged.IsBound())
		{
			for (int32 i = 0; i < Orbs; i++)
			{
				OnDamaged.Broadcast();
			}
		}
	}

	if (AEnemy* EnemyRef = Cast<AEnemy>(DamageCauser))
	{
		if (EnemyRef->GetPossessionComponent()->GetPossessedEntity())
		{
			EnemyRef->GetAttributeComponent()->IncreaseEnergy(Percentage);

			for (int32 i = 0; i < Orbs; i++)
			{
				OnDamaged.Broadcast();
			}
		}
	}
}

void AEnemy::GetFinished_Implementation()
{
	if (GetEnemyState() != EEnemyState::EES_Died)
	{
		if (GetCharacterStateComponent()) GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Dead);

		if (PromptWidgetComponent && PromptWidgetComponent->GetWidget()) PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

		FVector Start = GetActorLocation();
		FVector End = LastDamageCauser ? LastDamageCauser->GetActorLocation() : Start + GetActorForwardVector();
		FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		SetActorRotation(NewRotation);

		DisableAI();
		StopAnimMontage();
		PlayAnimMontage(FinisherDeathMontage, 1.f); 

		Die();
	}
}

bool AEnemy::IsLaunchable_Implementation(ACharacter* Character)
{
	return GetAttributeComponent()->IsShielded();
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
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, CollisionResponse);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, CollisionResponse);

	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, CollisionResponse);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, CollisionResponse); //sword trace
	//GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, CollisionResponse); //spectral weapon trace
}

void AEnemy::GetExecuted()
{
	Die();
	StopAnimMontage();
	PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));

	DropOrbs(25.f, GetPossessionComponent()->GetPossessingEntity());
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

EEnemyState AEnemy::SetEnemyState(EEnemyState NewState)
{
	EnemyState = NewState;

	return EnemyState;
}

void AEnemy::DisableAI()
{
	if (AIController)
	{
		AIController->StopMovement();
		AIController->UnPossess();
	}

	if (BBComponent)
	{
		BBComponent->ClearValue(FName("TargetActor"));
		BBComponent->ClearValue(FName("CanSeePlayer"));
		BBComponent->ClearValue(FName("DistToTarget"));
		BBComponent->ClearValue(FName("DamageTakenRecently"));
	}
	else
	{
		if (AIController)
		{
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}
		else
		{
			AIController = Cast<AAIController>(GetController());
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}

		if (BBComponent)
		{
			BBComponent->ClearValue(FName("TargetActor"));
			BBComponent->ClearValue(FName("CanSeePlayer"));
			BBComponent->ClearValue(FName("DistToTarget"));
			BBComponent->ClearValue(FName("DamageTakenRecently"));
		}
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
	}

	if (AIController)
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
	
	if (EnemyRef && EnemyRef->GetPossessionComponent()->GetPossessedEntity() || PlayerRef)
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

void AEnemy::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
	GetCombatComponent()->StartLaunchingUp();
}
