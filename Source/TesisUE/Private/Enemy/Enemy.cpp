#include "Enemy/Enemy.h"

#include "AIController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraComponent.h"
#include "AI/EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "DamageTypes/MeleeDamage.h"
#include "DataAssets/InputData.h"
#include "DataAssets/MontagesData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/Guid.h"
#include "Perception/AIPerceptionComponent.h"  
#include "Player/PlayerHeroController.h"
#include "Player/PlayerMain.h"
#include "SceneEvents/VelmaraGameModeBase.h"
#include "SceneEvents/VelmaraGameStateBase.h"
#include "Subsystems/EffectsManager.h"
#include "Subsystems/EnemyPoolManager.h"
#include "Subsystems/EnemyTokenManager.h"
#include "Tutorial/PromptWidgetComponent.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->CanCharacterStepUpOn = ECB_No;
	InitialMeshCollisionEnabled = GetMesh()->GetCollisionEnabled();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECB_No;
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

	CombatComponent->OnResetState.AddDynamic(this, &AEnemy::ReturnAttackTokenToTarget);

	GetAttributeComponent()->OnEntityDead.AddDynamic(this, &AEnemy::PerformDead);
	GetAttributeComponent()->OnDettachShield.AddDynamic(this, &AEnemy::NotifyIsNotShieldedToBlackboard);

	MementoComponent->bShouldSaveInventory = false;
}

void AEnemy::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);
	SetActorHiddenInGame(false);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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
	
	CharacterStateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);

	bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;

	if (AVelmaraGameModeBase* NewGameMode = Cast<AVelmaraGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		NewGameMode->RegisterEnemy(this);

		/*if (AVelmaraGameStateBase* NewGameStateBase = Cast<AVelmaraGameStateBase>(NewGameMode->GameState))
		{
			if (MementoComponent)
			{
				NewGameStateBase->RegisterMementoEntity(this);
			}
		}*/
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

	Execute_StopAnimMontage(this, GetCurrentMontage());
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

	MementoComponent->CaptureOwnerState();
	
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
	if (OnDead.IsBound()) OnDead.Broadcast(this);
	
	GetWorldTimerManager().SetTimer(ReturnToPoolTimerHandle, this, &AEnemy::RequestReturnToPool, 5.0f, false);
}

void AEnemy::GetHit(const TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	FDamageEvent const& DamageEvent, const float DamageReceived)
{
	Super::GetHit(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

	if (DamageEvent.DamageTypeClass)
	{
		if (const UMeleeDamage* MainDamageTypeClass = Cast<UMeleeDamage>(DamageEvent.DamageTypeClass->GetDefaultObject()))
		{
			const EMeleeDamageTypes MainDamageType = MainDamageTypeClass->DamageType;
			ReactToDamage(MainDamageType, ImpactPoint);

			if (bShouldDropOrbs) DropOrbs(DamageReceived, DamageCauser);
		}
	}
	
	NotifyDamageTakenToBlackboard(DamageCauser);
	ReturnAttackTokenToTarget();
	HitFlash(.1f,.75f);
}

void AEnemy::RequestReturnToPool()
{
	DeactivateEnemy();

	if (const UWorld* World = GetWorld())
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

	if (!AttributeComponent->IsAlive())
	{
		DeactivateEnemy();
	}
	
	PlayerControllerRef = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(this, 0));

	HandleEnemyCollision(true);
	
	CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	
	if (DissolveCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("UpdateDissolveEffect"));
		DissolveTimeline->AddInterpFloat(DissolveCurve, ProgressFunction);
	}

	if (AVelmaraGameModeBase* NewGameMode = Cast<AVelmaraGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		NewGameMode->RegisterEnemy(this);
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
		const TArray<UMaterialInterface*> TempArray = GetMesh()->GetMaterials();

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
	Die(MontagesData->Montages.DeathMontage, SelectRandomDieAnim());
}

void AEnemy::ReturnAttackTokenToTarget()
{
	if (const UWorld* World = GetWorld())
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
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Possess, ETriggerEvent::Started, GetPossessionComponent(), &UPossessionComponent::EjectPossessor);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_SwitchForm, ETriggerEvent::Started, GetPossessionComponent(), &UPossessionComponent::EjectAndExecute);
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

void AEnemy::NotifyThreat(const TScriptInterface<ICombatTargetInterface>& ThreatActor) const
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
		AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), ThreatActor.GetObject());
	}

	// a function that could forget the current target?
}

void AEnemy::OnPossessed()
{
	ReturnAttackTokenToTarget();
	DisableAI();
	ApplyPossessionParameters(true);
}

void AEnemy::OnUnpossessed()
{
	ReturnAttackTokenToTarget();
	EnableAI();
	ApplyPossessionParameters(false);

	if (IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
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

void AEnemy::HitFlash(const float Duration, const float Amount)
{
	for (UMaterialInstanceDynamic* DissolveMaterial : DissolveMaterials)
	{
		if (DissolveMaterial)
		{
			DissolveMaterial->SetScalarParameterValue(FName("HitFlashAmount"), Amount);
			GetWorldTimerManager().SetTimer(HitFlashTimerHandle, this,&AEnemy::DeactivateHitFlash, Duration, false);
		}
	}
}

void AEnemy::DeactivateHitFlash()
{
	for (UMaterialInstanceDynamic* DissolveMaterial : DissolveMaterials)
	{
		if (DissolveMaterial)
		{
			DissolveMaterial->SetScalarParameterValue(FName("HitFlashAmount"), 0);
		}
	}
}

void AEnemy::ResetEnemy()
{
	bIsLaunched = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	EnableAI();
}

void AEnemy::DropOrbs(const float DamageReceived, const TScriptInterface<ICombatTargetInterface>& DamageCauser) const
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Drops orbs called");
	const float Percentage = DamageReceived / EnergyDivider;
	const int32 Orbs = FMath::RoundToInt(Percentage) / 5;
	
	DamageCauser->IncreaseEnergy(Percentage);

	if (OnDamaged.IsBound())
	{
		for (int32 i = 0; i < Orbs; i++)
		{
			OnDamaged.Broadcast();
		}
	}
}

void AEnemy::FinishedDamage()
{
	if (!CanBeFinished()) return;
	
	if (IsAlive())
	{
		if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
		{
			EffectsManager->TimeWarp(ETimeWarpPreset::ETWP_Finisher);
			EffectsManager->CameraZoom(ECameraZoomPreset::ECZP_Finisher);
		}
		
		CharacterStateComponent->SetAction(ECharacterActionsStates::ECAS_Dead);
		Die(nullptr, NAME_None);

		if (PromptWidgetComponent && PromptWidgetComponent->GetWidget()) PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

		const FVector Start = GetTargetActorLocation();
		const FVector End = LastDamageCauser ? LastDamageCauser->GetTargetActorLocation() : Start + GetActorForwardVector();
		const FRotator NewRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		SetActorRotation(NewRotation);	
	}

	Execute_StopAnimMontage(this, GetCurrentMontage());
	Execute_PlayAnimMontage(this, FinisherDeathMontage, 1.f, "Default");
}

bool AEnemy::IsLaunchable()
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

		GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);

		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
}

void AEnemy::GetExecuted()
{
	Die(MontagesData->Montages.DeathMontage, FName("UnpossessDeath"));
	DropOrbs(30.f, GetPossessionComponent()->GetPossessor());
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

EEnemyState AEnemy::SetEnemyState(const EEnemyState NewState)
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

	if (!IsPossessed())
	{
		AIController->UnPossess();
		AIController->Possess(this);
	}

	EnemyAIController->PerceptionComponent->Activate();

	if (BTAsset)
	{
		AIController->RunBehaviorTree(BTAsset);
	}

	EnemyAIController->CustomInitialize(this, BBComponent, CharacterStateComponent);
}

TArray<TScriptInterface<ICombatTargetInterface>> AEnemy::GenerateSphereOverlapToDetectOtherEnemies(
	const FVector& Origin, const float Radius, AActor* HitEnemyToExclude)
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

	const bool bOverlapOccurred = UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		Origin,
		Radius,
		ObjectTypes,
		this->StaticClass(),
		ActorsToIgnoreForSphere,
		OverlappedActors
	);

	// DrawDebugSphere(
	// 	GetWorld(),
	// 	Origin,
	// 	Radius,
	// 	24,
	// 	FColor::Yellow,
	// 	false,
	// 	5.0f
	// );

	TArray<TScriptInterface<ICombatTargetInterface>> EnemiesFound;
	if (bOverlapOccurred)
	{
		for (AActor* Actor : OverlappedActors)
		{
			if (TScriptInterface<ICombatTargetInterface> CombatTarget = Actor)
			{
				EnemiesFound.Add(CombatTarget);
			}
		}
	}

	return EnemiesFound;
}

void AEnemy::NotifyDamageTakenToBlackboard(const TScriptInterface<ICombatTargetInterface>& DamageCauser)
{
	if (!DamageCauser) return;
	
	if (DamageCauser->IsPossessed())
	{
		if (AIController)
		{
			AIController->GetBlackboardComponent()->SetValueAsBool(FName("DamageTakenRecently"), true);
			AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), DamageCauser.GetObject());
		}

		if (EnemyAIController)
		{
			EnemyAIController->DamageCauser = DamageCauser;
		}

		for (TScriptInterface CombatTarget  : GenerateSphereOverlapToDetectOtherEnemies(GetTargetActorLocation(), RadiusToNotifyAllies, this))
		{
			Cast<AEnemy>(CombatTarget.GetObject())->NotifyThreat(DamageCauser);
		}
	}
}
