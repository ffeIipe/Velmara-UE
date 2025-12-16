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
#include "Components/PossessionComponent.h"
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

	//UniqueSaveID = FName(*FGuid::NewGuid().ToString());
	
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

	/*SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	SpringArm->SocketOffset = FVector(0.f, 0.f, 45.f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;*/

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
	/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::White, FString("Deactivating " + this->GetName()));*/
	/*if (GetPossessionComponent()->GetPossessionOwner())
	{
		APlayerMain* PlayerOwnerRef = Cast<APlayerMain>(GetPossessionComponent()->GetPossessionOwner());
		if (PlayerOwnerRef && IsValid(PlayerOwnerRef))
		{
			UnPossessBase();
		}
		PossessionOwner = nullptr;
	}*/

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

void AEnemy::Die(/*AActor* DamageCauser*/)
{
	Super::Die(/*DamageCauser*/);

	DissolveTimeline->PlayFromStart(); //enemy

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

	if (GetPossessionComponent()->GetPossessionOwner())
	{
		GetPossessionComponent()->ReleasePossession();
	}

	if (OnDeactivated.IsBound())
	{
		OnDeactivated.Broadcast(this);
	}

	GetWorldTimerManager().SetTimer(ReturnToPoolTimerHandle, this, &AEnemy::RequestReturnToPool, 5.0f, false);
}

void AEnemy::PoolableDie(/*AActor* DamageCauser*/)
{
	Die(/*DamageCauser*/);
}

void AEnemy::RequestReturnToPool()
{
	/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, FString("Request return to pool..."));*/

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

	GetAttributeComponent()->OnOutOfEnergy.AddLambda(
		[this]
		{
			if (GetPossessionComponent()->GetPossessionOwner())
			{
				GetPossessionComponent()->ReleasePossession();
			}
		}
	);

	PlayerControllerRef = Cast<APlayerHeroController>(UGameplayStatics::GetPlayerController(this, 0));

	HandleEnemyCollision(ECR_Block);

	GetDefaultParameters();

	GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippedSword);

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

void AEnemy::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	Super::GetHit_Implementation(DamageCauser, ImpactPoint, DamageType, DamageReceived);

	ReactToDamage(LastDamageType, ImpactPoint);

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
		if (EnemyRef->GetPossessionComponent()->GetPossessionOwner())
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

bool AEnemy::CanBeFinished_Implementation()
{
	return GetAttributeComponent()->GetHealthPercent() <= .2f;
}

bool AEnemy::IsLaunchable_Implementation(ACharacter* Character)
{
	return false;
}

//float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
//{
//	/*if (EnemyState == EEnemyState::EES_Died)
//	{
//		return 0.f;
//	}
//
//	const UDamageTypeMain* MainDamageType = DamageEvent.DamageTypeClass
//		? Cast<UDamageTypeMain>(DamageEvent.DamageTypeClass->GetDefaultObject())
//		: nullptr;
//
//	LastDamageType = MainDamageType
//		? MainDamageType->DamageType
//		: EMainDamageTypes::EMDT_None;*/
//
//
//	if (GetAttributeComponent())
//	{
//		if (GetAttributeComponent()->IsAlive())
//		{
//			GetAttributeComponent()->ReceiveDamage(DamageAmount);
//			NotifyDamageTakenToBlackboard(DamageCauser);
//
//			if (Execute_CanBeFinished(this))
//			{
//				if (PromptWidgetComponent && PromptWidgetComponent->GetWidget())
//				{
//					PromptWidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Visible);
//					PromptWidgetComponent->LoadAndApplyPrompt();
//				}
//			}
//		}
//	}
//	return DamageAmount;
//}

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
	//GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, CollisionResponse); //spectral weapon trace
	//GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, CollisionResponse);
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
		else return;
	}

	if (AIController)
	{
		if (!GetPossessionComponent()->GetPossessionOwner())
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

//void AEnemy::OnPossessed(AEntity* NewOwner, float OwnerEnergy)
//{
//	if (!IsValid(NewOwner)) return;
//
//	PossessionOwner = NewOwner;
//
//	GetAttributeComponent()->SetEnergy(OwnerEnergy);
//
//	bUseControllerRotationYaw = false;
//	
//	SetOnPossessedParameters();
//
//	PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(true);
//
//	if (GetAttributeComponent())
//	{
//		GetAttributeComponent()->StartDecreaseEnergy();
//		GetAttributeComponent()->OnDepletedCallback = [this, WeakOwningPlayer = TWeakObjectPtr<APlayerMain>(NewOwner)]() {
//			if (WeakOwningPlayer.IsValid())
//			{
//				UnPossess();
//			}
//			else
//			{
//				if (PossessionOwner == WeakOwningPlayer.Get(true))
//				{
//					UnPossess();
//					PossessionOwner = nullptr;
//				}
//			}
//			};
//	}
//}

//void AEnemy::UnPossessBase()
//{
//	bUseControllerRotationYaw = true;
//
//	GetDefaultParameters(); // gettear y settear parametros por defecto... se puede mejorar con algo mas lindo como un struct que gettee y settee... 
//
//	PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(false);
//
//	if (PossessionOwner)
//	{
//		if (IsValid(PossessionOwner))
//		{
//			PossessionOwner->ReleasePossession(this);
//		}
//	}
//
//	if (AIController)
//	{
//		AIController->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
//		AIController->GetBlackboardComponent()->ClearValue(FName("CanSeePlayer"));	
//	}
//}

//void AEnemy::UnPossess()
//{
//	if (PossessionOwner)
//	{
//		if (PossessionOwner->GetAttributesComponent())
//		{
//			UnPossessBase();
//			PossessionOwner->GetAttributes()->SetEnergy(GetAttributeComponent()->GetEnergy());
//			PossessionOwner = nullptr;
//			EnableAI();
//		}
//	}
//	else PossessionOwner = nullptr;
//}
//
//void AEnemy::UnPossessAndKill()
//{
//	if (PossessionOwner && PossessionOwner->GetAttributes() && PossessionOwner->GetAttributes()->RequiresEnergy(UnpossesAndKillEnergyTax))
//	{
//		PossessionOwner->Attributes->IncreaseHealth(15.f);
//		GetAttributeComponent()->IncreaseEnergy(-UnpossesAndKillEnergyTax);
//
//		DisableAI();
//		StopAnimMontage();
//		PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));
//
//		LastDamageCauser = PossessionOwner;
//
//		Die(LastDamageCauser);
//
//		float Orbs = FMath::RoundToInt(15.f) / 5;
//
//		if (OnDead.IsBound())
//		{
//			for (int32 i = 0; i < Orbs; i++)
//			{
//				OnDead.Broadcast();
//			}
//		}
//
//		if (IsValid(PossessionOwner))
//		{
//			if (IsValid(PossessionOwner->CharacterStateComponent))
//			{
//				PossessionOwner->ToggleForm();
//			}
//		}
//
//		UnPossessBase();
//		PossessionOwner = nullptr;
//	}
//	else if (GetAttributeComponent() && !GetAttributeComponent()->RequiresEnergy(UnpossesAndKillEnergyTax) && ErrorSFX)
//	{
//		UGameplayStatics::PlaySound2D(GetWorld(), ErrorSFX);
//	}
//}

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
	
	if (EnemyRef && EnemyRef->GetPossessionComponent()->GetPossessionOwner() || PlayerRef)
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
	
//void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
//
//	if (!EnhancedInputComponent) return;
//
//	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEnemy::Move);
//	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEnemy::Look);
//	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEnemy::Jump);
//	EnhancedInputComponent->BindAction(ExtraMovementComponent->DodgeAction, ETriggerEvent::Triggered, this, &AEnemy::Dodge);
//
//	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AEnemy::Attack);
//
//	EnhancedInputComponent->BindAction(UnPossessAction, ETriggerEvent::Completed, this, &AEnemy::UnPossess);
//	EnhancedInputComponent->BindAction(UnPossessAndKillAction, ETriggerEvent::Completed, this, &AEnemy::UnPossessAndKill);
//}