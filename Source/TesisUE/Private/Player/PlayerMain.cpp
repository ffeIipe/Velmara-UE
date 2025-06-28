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

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());

	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttibuteComponent"));

	MementoComponent = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	CombatComponent->FinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FinisherLocation"));
	CombatComponent->FinisherLocation->SetupAttachment(GetMesh());
	CombatComponent->FinisherLocation->SetRelativeLocation(FVector(-8.f, 80.f, 0.f));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterStates"));

	SpectralWeaponComponent = CreateDefaultSubobject<USpectralWeaponComponent>(TEXT("SpectralWeapon"));
	//SpectralWeaponComponent->GetSpectralWeaponMeshComponent()->SetupAttachment(GetMesh(), FName("RightHandSocketWeapon"));

	ExtraMovementComponent = CreateDefaultSubobject<UExtraMovementComponent>(TEXT("ExtraMovementComponent"));
}

void APlayerMain::PerformSpectralAttack_Implementation()
{
	SpectralWeaponComponent->PrimaryFire();
}

void APlayerMain::PerformSpectralBarrier_Implementation()
{
	SpectralWeaponComponent->SecondaryFire();
}

void APlayerMain::ResetSpectralAttack_Implementation()
{
	SpectralAttackIndex = 0;
	CombatComponent->bIsSaveLightAttack = false;
}

void APlayerMain::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Dead })) return;

	/*if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun }))
	{
		if (DamageType != USpectralTrapDamageType::StaticClass())
		{
			return;
		}
	}*/

	if (ReceiveDamageSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ReceiveDamageSFX, GetActorLocation());
	}

	if (DamageType == USpectralTrapDamageType::StaticClass())
	{
		//CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Stun);
		StunBehavior();
	}
	else
	{
		CombatComponent->GetDirectionalReact(ImpactPoint, DamageType);
		//RemoveStunBehavior(); no sirve esto porque al pegarte el boss te saca el estado de stun
	}
}

UCharacterStateComponent* APlayerMain::GetCharacterStateComponent_Implementation()
{
	return CharacterStateComponent;
}

UMementoComponent* APlayerMain::GetMementoComponent_Implementation()
{
	return MementoComponent;
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

	Attributes->OnEntityDead.AddLambda(
		[this]
		{
			Die();
		}
	);

	CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral ?
		SpectralWeaponComponent->EnableSpectralWeapon(true) : SpectralWeaponComponent->EnableSpectralWeapon(false);

	CombatComponent->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);

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

void APlayerMain::StunBehavior()
{
	GetMesh()->GlobalAnimRateScale = .5f;
	GetCharacterMovement()->MaxWalkSpeed = StunMaxWalkSpeed;
}

void APlayerMain::RemoveStunBehavior()
{
	GetMesh()->GlobalAnimRateScale = 1.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
}

float APlayerMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!bCanReceiveDamage) return 0.f;

	if (Attributes && Attributes->IsAlive())
	{
		Attributes->ReceiveDamage(DamageAmount);
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
		Start = FollowCamera->GetActorLocation() + FollowCamera->GetActorForwardVector() * 100.f;
		End = Start + FollowCamera->GetActorForwardVector() * PossessDistance;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Empty();
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		Start,
		End,
		InteractTargetRadius,
		ETraceTypeQuery::TraceTypeQuery4,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		ResultHit,
		true
	);

	if (bHit)
	{

		if (IHitInterface* Entity = Cast<IHitInterface>(ResultHit.GetActor()))
		{
			if (Entity->Execute_IsLaunchable(ResultHit.GetActor(), this))
			{
				//esto es una garompa, se soluciona de otra manera. A continuacion el 'susodicho'
				if (Cast<APaladinBoss>(ResultHit.GetActor())) return nullptr;

				else return Cast<AEnemy>(ResultHit.GetActor());
			}
			else return nullptr;
		}
		else return nullptr;
	}
	else return nullptr;
}

void APlayerMain::PossessEnemy()
{
	if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral && !IsEquipping())
	{
		AEnemy* TargetEnemy = GetTargetEnemy();
		PlayerControllerRef = Cast<APlayerController>(GetController());

		if (PlayerControllerRef && TargetEnemy && TargetEnemy->GetEnemyState() != EEnemyState::EES_Died && Attributes->RequiresEnergy(10.f))
		{
			TargetEnemy->DisableAI();
			PlayerControllerRef->Possess(TargetEnemy);
			PossessedEnemy = TargetEnemy;

			TargetEnemy->EnableInput(PlayerControllerRef);
			TargetEnemy->AutoPossessPlayer = EAutoReceiveInput::Player0;
			TargetEnemy->OnPossessed(this, Attributes->GetEnergy());

			FollowCamera->AttachToComponent(TargetEnemy->SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
			PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);

			CharacterStateComponent->SetCharacterForm(ECharacterForm::ECF_Possessing);

			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			GetMesh()->bPauseAnims = true;
		}
		else if (!Attributes->RequiresEnergy(10.f) && ErrorSFX)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), ErrorSFX);
		}
	}
	else CombatComponent->Execute();
}

void APlayerMain::ReleasePossession(AEnemy* EnemyBeingUnpossessed)
{
	CharacterStateComponent->SetCharacterForm(ECharacterForm::ECF_Spectral);

	if (PlayerControllerRef)
	{
		PlayerControllerRef->Possess(this);

		if (IsValid(this->PossessedEnemy))
		{
			this->PossessedEnemy->DisableInput(PlayerControllerRef);
			this->PossessedEnemy->EnableAI();
		}

		if (IsValid(FollowCamera) && IsValid(CameraBoom))
		{
			FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
			PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
		}
	}


	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (IsValid(EnemyBeingUnpossessed))
	{
		SetActorLocation(EnemyBeingUnpossessed->GetActorLocation());
		SetActorRotation(EnemyBeingUnpossessed->GetActorRotation());
	}
	else if (IsValid(this->PossessedEnemy))
	{
		SetActorLocation(this->PossessedEnemy->GetActorLocation());
		SetActorRotation(this->PossessedEnemy->GetActorRotation());
	}

	if (GetMesh())
	{
		GetMesh()->bPauseAnims = false;
	}

	this->PossessedEnemy = nullptr;
}


void APlayerMain::Move(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun }))
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

void APlayerMain::Dodge(const FInputActionValue& Value)
{
	if (!IsEquipping())
	{
		ExtraMovementComponent->Input_Dodge();
	}
	
}

bool APlayerMain::IsEquipping()
{
	return CharacterStateComponent->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_EquippingPistol
		|| CharacterStateComponent->GetCurrentCharacterState().State == ECharacterStates::ECS_EquippingSword;
}

void APlayerMain::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void APlayerMain::Jump()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Nothing, ECharacterActions::ECA_Attack }) && ExtraMovementComponent->CanDoubleJump && !IsEquipping())
	{
		PlayAnimMontage(ExtraMovementComponent->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && ExtraMovementComponent->CanDoubleJump)
		{
			ExtraMovementComponent->Input_DoubleJump();
		}
	}
}

void APlayerMain::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	CombatComponent->bIsLaunched = false;
	ExtraMovementComponent->CanDoubleJump = true;
}

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (InventoryComponent && InventoryComponent->IsInventoryOpen() && IsEquipping()) return;
	
	FVector TraceStart;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(TraceStart, CameraRotation);

	FVector TraceDirection = CameraRotation.Vector();
	FVector TraceEnd = TraceStart + (TraceDirection * InteractTraceLenght);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		InteractTargetRadius,
		ETraceTypeQuery::TraceTypeQuery1, //visibility trace
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		ResultHit,
		true
	);


	if (bHit && InventoryComponent)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString(ResultHit.GetActor()->GetName()));

		if (ASword* HitSword = Cast<ASword>(ResultHit.GetActor()))
		{
			if (CharacterStateComponent->GetCurrentCharacterState().Form != ECharacterForm::ECF_Spectral)
			{
				if (InventoryComponent->TryAddItem(HitSword))
				{
					ActorsToIgnore.Add(HitSword);
					HitSword->OnWallHit.AddDynamic(this, &APlayerMain::OnWallCollision);

					Equipping(true);
				}
			}
		}
		else if (ISpectralInteractable* SpectralObjectInteractable = Cast<ISpectralInteractable>(ResultHit.GetActor()))
		{
			if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
			{
				SpectralObjectInteractable->Execute_SpectralInteract(ResultHit.GetActor(), this);
			}
		}
		else if (AItem* HitItem = Cast<AItem>(ResultHit.GetActor()))
		{
			HitItem->Use(this);
			Equipping(false);
		}
	}
}

void APlayerMain::Equipping(bool bIsSwordBeingEquipped)
{
	ECharacterForm ActualForm = CharacterStateComponent->GetCurrentCharacterState().Form;

	if (ActualForm == ECharacterForm::ECF_Human) //si estoy en humano
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("Human form"));

		if (bIsSwordBeingEquipped) //y estoy equipando una espada
		{
			if (InventoryComponent->GetEquippedItem())
			{
				if (SpectralWeaponComponent->bWasInitialized && CharacterStateComponent->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_EquippedPistol)
				{
					PlayAnimMontage(EquipPistolMontage, 1.f, FName("BackToSword"));
					CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippingSword);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, FString("Equipping sword"));

					PlayAnimMontage(EquipSwordMontage, 1.f, FName("Equip"));
					CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippingSword);
				}
			}
			else GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan, FString("No equipped item, so none animation will be played..."));
		}
		else //o estoy equipando una pistola
		{
			if (!InventoryComponent->GetEquippedItem()) //si no tengo espada
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange, FString("Only pick pistol"));

				PlayAnimMontage(EquipPistolMontage, 1.f, FName("Unequip"));
				CharacterStateComponent->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
			}
			else
			{
				//si tengo espada equipada
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("Unequip sword and equip pistol"));

				PlayAnimMontage(EquipSwordMontage, 1.f, FName("PickPistol"));
				CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippingSword);
			}
		}
	}
	else //si no estoy en humano
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Purple, FString("Spectral form"));

		if (!bIsSwordBeingEquipped)
		{
			if (InventoryComponent->GetEquippedItem())
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan, FString("Has sword"));

				PlayAnimMontage(EquipSwordMontage, 1.f, FName("SwitchToPistol"));
				CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippingSword);
			}
			else if (SpectralWeaponComponent->bWasInitialized)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Blue, FString("Has pistol"));

				PlayAnimMontage(EquipPistolMontage, 1.f, FName("Equip"));
				CharacterStateComponent->SetCharacterSpectralState(ECharacterSpectralStates::ECSS_EquippingPistol);
			}
			
		}
	}	
}

void APlayerMain::Attack(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_Attack(Value);
	}
}

void APlayerMain::HeavyAttack(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_HeavyAttack(Value);
	}
}

void APlayerMain::LaunchAttack(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_Launch(Value);
	}
}

void APlayerMain::Block(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_Block(Value);
	}
}

void APlayerMain::ReleaseBlock(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_ReleaseBlock(Value);
	}
}

void APlayerMain::Execute(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		CombatComponent->Input_Execute(Value);
	}
}

void APlayerMain::ToggleForm()
{
	if (!IsValid(CharacterStateComponent)) return;
	
	if (!GetWorld()) return;

	if (!IsValid(Attributes)) return;

	if (IsEquipping()) return;

	if (CharacterStateComponent->IsActionEqualToAny({
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

	if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
	{
		Attributes->StartDecreaseEnergy();
		WithEnergy();
		Attributes->StopRegenerateTick();
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
	}
}

void APlayerMain::OutOfEnergy()
{
	PlayerFormComponent->ToggleForm(false);
	if (Attributes)
	{
		Attributes->RegenerateTick();
	}

	if (GetCharacterMovement() && GetCharacterMovement()->GetPawnOwner())
	{
		GetCharacterMovement()->GetPawnOwner()->bUseControllerRotationYaw = false;
	}

	if (SpectralWeaponComponent)
	{
		SpectralWeaponComponent->EnableSpectralWeapon(false);
	}

	if (InventoryComponent && InventoryComponent->GetEquippedItem())
	{
		InventoryComponent->GetEquippedItem()->EnableVisuals(true);
	}

	if (this->PossessedEnemy)
	{
		if (IsValid(this->PossessedEnemy))
		{
			this->PossessedEnemy->UnPossess();
		}
		else this->PossessedEnemy = nullptr;
		
	}
}

void APlayerMain::Die()
{
	if (!bIsDead)
	{
		bIsDead = true;

		GetCharacterMovement()->DisableMovement();
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Dead);

		if (DeathMontage)
		{
			StopAnimMontage();
			PlayAnimMontage(DeathMontage);
		}
		
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController)
		{
			DisableInput(PlayerController);
		}
		
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
			CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		}
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SprinEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::RestartLevel()
{	
	FName CurrentLevel = *UGameplayStatics::GetCurrentLevelName(this);
	UGameplayStatics::OpenLevel(this, FName("Felipe"));
}

void APlayerMain::OnWallCollision(const FHitResult& HitResult)
{
	StopAnimMontage();
	CombatComponent->HitReactJumpToSection(FName("ReactToShield"));
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
	if (CharacterStateComponent->IsFormEqualToAny({ ECharacterForm::ECF_Spectral })) return;

	InventoryComponent->ChangeWeapon(0);
	Equipping(true);
}

void APlayerMain::ChangeSecondaryWeapon()
{
	if (CharacterStateComponent->IsFormEqualToAny({ ECharacterForm::ECF_Spectral })) return;

	InventoryComponent->ChangeWeapon(1);
	Equipping(true);
}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
		EnhancedInputComponent->BindAction(ExtraMovementComponent->JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
		EnhancedInputComponent->BindAction(ExtraMovementComponent->DodgeAction, ETriggerEvent::Started, this, &APlayerMain::Dodge);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerMain::Interact);

		EnhancedInputComponent->BindAction(CombatComponent->AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
		EnhancedInputComponent->BindAction(CombatComponent->HeavyAttackAction, ETriggerEvent::Started, this, &APlayerMain::HeavyAttack);
		EnhancedInputComponent->BindAction(CombatComponent->LaunchAction, ETriggerEvent::Triggered, this, &APlayerMain::LaunchAttack);
		//EnhancedInputComponent->BindAction(CombatComponent->BlockAction, ETriggerEvent::Started, this, &APlayerMain::Block);
		//EnhancedInputComponent->BindAction(CombatComponent->BlockAction, ETriggerEvent::Completed, this, &APlayerMain::ReleaseBlock);

		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Completed, this, &APlayerMain::PossessEnemy);
		
		//EnhancedInputComponent->BindAction(InventoryComponent->Slot1_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangePrimaryWeapon);
		//EnhancedInputComponent->BindAction(InventoryComponent->Slot2_InventoryAction, ETriggerEvent::Started, this, &APlayerMain::ChangeSecondaryWeapon);
	}
}