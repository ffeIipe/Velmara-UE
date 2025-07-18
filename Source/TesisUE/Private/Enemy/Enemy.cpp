#include "Enemy/Enemy.h"
#include "AI/EnemyAIController.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
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
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/Guid.h"
#include "NiagaraComponent.h"
#include "Player/PlayerMain.h"
#include "SceneEvents/NewGameModeBase.h"
#include "SceneEvents/NewGameStateBase.h"
#include "Subsystems/EnemyPoolManager.h"
#include "Subsystems/EnemyTokenManager.h"
#include "Tutorial/PromptWidgetComponent.h"
#include "Player/PlayerHeroController.h"
#include "Perception/AIPerceptionComponent.h"  

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

	OnCanBeFinished.AddDynamic(this, &AEnemy::EnableFinisherWidget);

	GetPossessionComponent()->OnPossessed.AddDynamic(this, &AEnemy::OnPossessed);
	GetPossessionComponent()->OnPossessorEjected.AddDynamic(this, &AEnemy::OnUnpossessed);
	GetPossessionComponent()->OnPossessorExecutedMeAndEjected.AddDynamic(this, &AEnemy::GetExecuted);

	GetCombatComponent()->OnAttackEnd.AddDynamic(this, &AEnemy::ReturnAttackTokenToTarget);

	GetAttributeComponent()->OnEntityDead.AddDynamic(this, &AEnemy::PerformDead);
	GetAttributeComponent()->OnDettachShield.AddDynamic(this, &AEnemy::NotifyIsNotShieldedToBlackboard);
}

void AEnemy::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);
	SetActorHiddenInGame(false);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	GetMesh()->bPauseAnims = false;

	HandleEnemyCollision(true);
	ApplyPossessionParameters(false);

	EnemyState = EEnemyState::EES_None;
	bIsLaunched = false;
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
	HandleEnemyCollision(false);

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
	}

	DisableAI();

	bIsLaunched = false;
	LastDamageCauser = nullptr;
	
	if (OnDeactivated.IsBound())
	{
		OnDeactivated.Broadcast(this);
	}
	
	GetWorldTimerManager().ClearTimer(HitFlashTimerHandle);
	GetWorldTimerManager().ClearTimer(ReturnToPoolTimerHandle);
}

void AEnemy::Die(UAnimMontage* DeathAnim, FName Section)
{
	Super::Die(DeathAnim, Section);

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

	//disable 'F' widget
	if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
	{
		PromptWidgetComponent->GetPromptWidgetComponent()->EnablePromptWidget(false);
	}

	DisableAI();
	ReturnAttackTokenToTarget();
	HandleEnemyCollision(false);

	//release possessor
	if (GetPossessionComponent()->GetPossessedEntity())
	{
		GetPossessionComponent()->ReleasePossession();
	}

	if (OnDeactivated.IsBound()) OnDeactivated.Broadcast(this);
	
	GetWorldTimerManager().SetTimer(ReturnToPoolTimerHandle, this, &AEnemy::RequestReturnToPool, 5.0f, false);
}

void AEnemy::RequestReturnToPool()
{
	DeactivateEnemy();

	if (UWorld* World = GetWorld())
	{
		if (UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>())
		{
			PoolManager->ReturnEnemyToPool(this);
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

	HandleEnemyCollision(true);

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

	if (GetMesh())
	{
		TArray<UMaterialInterface*> TempArray = GetMesh()->GetMaterials();

		for (int32 MatIndex = 0; MatIndex < TempArray.Num(); MatIndex++)
		{
			UMaterialInterface* CurrentMaterial = GetMesh()->GetMaterial(MatIndex);
			DissolveMaterials.Add(UMaterialInstanceDynamic::Create(CurrentMaterial, this));
			GetMesh()->SetMaterial(MatIndex, DissolveMaterials[MatIndex]);
		}
	}
	
	EnableAI();
}

void AEnemy::PerformDead()
{
	Die(DeathMontage, SelectRandomDieAnim());
}

void AEnemy::ReturnAttackTokenToTarget()
{
	if (UWorld* World = GetWorld())
	{
		if (UEnemyTokenManager* TokenManager = World->GetSubsystem<UEnemyTokenManager>())
		{
			TokenManager->ReturnAttackToken();
		}
	}

	EnemyAIController = Cast<AEnemyAIController>(AIController);
	if (EnemyAIController)
	{
		EnemyAIController->SetHasReservedAttackToken(false);
	}
}

void AEnemy::NotifyIsNotShieldedToBlackboard()
{
	if (!AIController) AIController = Cast<AAIController>(GetController());

	if (!BBComponent) BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());

	BBComponent->SetValueAsBool(FName("IsShielded"), false);
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

void AEnemy::NotifyThreat(AEntity* ThreatActor)
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
	}

	// una func con un forget?
}

void AEnemy::OnPossessed()
{
	ReturnAttackTokenToTarget();
	DisableAI();
	ApplyPossessionParameters(true);
	GetExtraMovementComponent()->CustomInitialize(this, GetCharacterStateComponent());
}

void AEnemy::OnUnpossessed()
{
	ReturnAttackTokenToTarget();
	EnableAI();
	ApplyPossessionParameters(false);
	GetExtraMovementComponent()->CustomInitialize(this, GetCharacterStateComponent());

	IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(GetController());
	if (OtherTeamAgent)
	{
		OtherTeamAgent->SetGenericTeamId(FGenericTeamId(0));
	}
}

void AEnemy::UpdateDissolveEffect(float Value)
{
	const float ClampedValue = FMath::Clamp(Value, 0.f, 1.f);

	for (UMaterialInstanceDynamic* DissolveMaterial : DissolveMaterials)
	{
		if (IsValid(DissolveMaterial))
		{
			DissolveMaterial->SetScalarParameterValue(FName("Animation"), ClampedValue);
		}
	}

	if (IsValid(DissolveParticleComponent))
	{
		DissolveParticleComponent->SetVariableFloat(FName("User_Animation"), ClampedValue);
	}
}

void AEnemy::ResetEnemy()
{
	bIsLaunched = false;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	EnableAI();
}

void AEnemy::GetHit_Implementation(AEntity* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived)
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

	if (DamageCauser)
	{
		if (bShouldDropOrbs)
		{
			DropOrbs(DamageReceived, DamageCauser);
		}
		NotifyDamageTakenToBlackboard(DamageCauser);
	}
}

void AEnemy::DropOrbs(const float DamageReceived, AEntity* DamageCauser) const
{
	const float Percentage = DamageReceived / EnergyDivider;
	const int32 Orbs = FMath::RoundToInt(Percentage) / 5;

	if (const APlayerMain* PlayerRef = Cast<APlayerMain>(DamageCauser))
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

	if (const AEnemy* EnemyRef = Cast<AEnemy>(DamageCauser))
	{
		if (EnemyRef->GetPossessionComponent()->IsPossessed())
		{
			EnemyRef->GetAttributeComponent()->IncreaseEnergy(Percentage);

			for (int32 i = 0; i < Orbs; i++)
			{
				OnDamaged.Broadcast();
			}
		}
	}
}

void AEnemy::FinishedDamage()
{
	if (GetCharacterStateComponent()->GetCurrentCharacterState().Action != ECharacterActions::ECA_Dead)
	{
		GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Dead);
		Die(nullptr, NAME_None);
		StopAnimMontage();
		PlayAnimMontage(FinisherDeathMontage);

		if (PromptWidgetComponent && PromptWidgetComponent->GetWidget()) PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

		FVector Start = GetActorLocation();
		FVector End = LastDamageCauser ? LastDamageCauser->GetActorLocation() : Start + GetActorForwardVector();
		FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		SetActorRotation(NewRotation);	
	}
}

bool AEnemy::IsLaunchable_Implementation()
{
	return !GetAttributeComponent()->IsShielded() && GetAttributeComponent()->IsAlive();
}

void AEnemy::HandleEnemyCollision(bool bEnable)
{
	if (bEnable)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore); 
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetMesh()->SetCollisionObjectType(ECC_Pawn);
		GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionObjectType(ECC_WorldDynamic);
		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	else
	{
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

		GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);//

		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

void AEnemy::GetExecuted()
{
	Die(DeathMontage, FName("UnpossessDeath"));

	DropOrbs(30.f, GetPossessionComponent()->GetPossessingEntity());
}	

FName AEnemy::SelectRandomDieAnim()
{
	const int32 RandomValue = FMath::RandRange(0, 3);
	FName SectionName = FName();

	switch (RandomValue)
	{
	case 0:
		return SectionName = FName("Death1");
	case 1:
		return SectionName = FName("Death2");
	case 2:
		return SectionName = FName("Death3");
	case 3:
		return SectionName = FName("Death4");
	default:
		return FName("");
	}
}

EEnemyState AEnemy::SetEnemyState(EEnemyState NewState)
{
	EnemyState = NewState;

	return EnemyState;
}

void AEnemy::DisableAI()
{
	if (!GetPossessionComponent()->IsPossessed())
	{
		if (!AIController) AIController = Cast<AAIController>(GetController());
			AIController->StopMovement();
			AIController->UnPossess();
			AIController->RunBehaviorTree(nullptr);

		if (!EnemyAIController) EnemyAIController = Cast<AEnemyAIController>(AIController);
			EnemyAIController->DeactivateController();
	}
	
	ClearBlackboardValues();
}

void AEnemy::ClearBlackboardValues()
{
	if (!BBComponent) BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());

	BBComponent->ClearValue(FName("TargetActor"));
	BBComponent->ClearValue(FName("DistToTarget"));
	BBComponent->ClearValue(FName("DamageTakenRecently"));
}

void AEnemy::EnableAI()
{
	if (!AIController) AIController = Cast<AAIController>(GetController());

	if (!EnemyAIController) EnemyAIController = Cast<AEnemyAIController>(AIController);

	if (!GetPossessionComponent()->IsPossessed())
	{
		AIController->UnPossess();
		AIController->Possess(this);
	}

	EnemyAIController->PerceptionComponent->Activate();

	if (BTAsset)
	{
		AIController->RunBehaviorTree(BTAsset);
	}

	EnemyAIController->CustomInitialize(this, BBComponent, GetCharacterStateComponent());
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

void AEnemy::NotifyDamageTakenToBlackboard(AEntity* DamageCauser)
{
	AEnemy* IsEnemyDamageCauser = Cast<AEnemy>(DamageCauser);
	APlayerMain* IsPlayerDamageCauser = Cast<APlayerMain>(DamageCauser);
	
	if (IsEnemyDamageCauser && IsEnemyDamageCauser->GetPossessionComponent()->IsPossessed() || IsPlayerDamageCauser)
	{
		if (AIController)
		{
			AIController->GetBlackboardComponent()->SetValueAsBool(FName("DamageTakenRecently"), true);
			AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), DamageCauser);
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
