// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerMain.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/PlayerFormComponent.h"
#include "SpectralMode/Interfaces/SpectralInteractable.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Enemy/Spectre.h"
#include "Enemy/Enemy.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Enemy/Paladin.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameModeBase.h"

APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));

	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttibuteComponent"));

	FinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FinisherPosition"));
	FinisherLocation->SetupAttachment(GetMesh());

	CameraFinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("CameraFinisherLocation"));
	CameraFinisherLocation->SetupAttachment(GetMesh());

	FinisherCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FinisherCollision"));
	FinisherCollision->SetupAttachment(GetMesh());

	AutoPossessPlayer = EAutoReceiveInput::Player0;
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

		FOnTimelineFloat ProgressAttackFunction;
		ProgressAttackFunction.BindUFunction(this, FName("UpdateAttackBuffer"));
		BufferAttackTimeline->AddInterpFloat(BufferCurve, ProgressAttackFunction);
	}

	if (SoftLockCurve)
	{
		FOnTimelineFloat ProgressSoftLockFunction;
		ProgressSoftLockFunction.BindUFunction(this, FName("UpdateSoftLockOn"));
		SoftLockTimeline->AddInterpFloat(SoftLockCurve, ProgressSoftLockFunction);
	}
}

void APlayerMain::PerformLightAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();
		PlayAnimMontage(LightAttackCombo[AttackIndex]);

		LightAttackIndex++;

		if (LightAttackIndex >= LightAttackCombo.Num())
		{
			LightAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformSpectralAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SearchTarget();
		PlayAnimMontage(SpectralAttackCombo[AttackIndex]);

		SpectralAttackIndex++;

		if (SpectralAttackIndex >= SpectralAttackCombo.Num())
		{
			SpectralAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformSpectralBarrier()
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		SetCharacterAction(ECharacterActions::ECA_Attack);
		PlayAnimMontage(SpectralHeavyAttack);
	}
}

void APlayerMain::PerformJumpAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();
		PlayAnimMontage(JumpAttackCombo[AttackIndex]);

		JumpAttackIndex++;

		if (JumpAttackIndex >= JumpAttackCombo.Num())
		{
			JumpAttackIndex = 0;
			PlayAnimMontage(CrasherMontage, 1.f);
			isLaunched = false;
		}
	}
}

void APlayerMain::PerformComboStarter(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterAction() != ECharacterActions::ECA_Dodge && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		ComboExtenderIndex = AttackIndex;
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);	
		PlayAnimMontage(ComboStarterAttack[AttackIndex - 1]);

		IsSaveHeavyAttack = false;
		IsSaveLightAttack = false;

		SoftLockOn();
	}
}

void APlayerMain::PerformComboExtender(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterAction() != ECharacterActions::ECA_Dodge && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		PlayAnimMontage(ComboExtenderAttack[AttackIndex - 1]);
		ResetLightAttackStats();
		ResetHeavyAttackStats();
		SoftLockOn();
	}
}

void APlayerMain::PerformHeavyAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		PlayAnimMontage(HeavyAttackCombo[AttackIndex]);
		SoftLockOn();

		HeavyAttackIndex++;

		if (HeavyAttackIndex >= HeavyAttackCombo.Num())
		{
			HeavyAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformDodge()
{
	if (PlayerFormComponent && PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		if (GetCharacterAction() == ECharacterActions::ECA_Finish) return;
		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		SetCharacterAction(ECharacterActions::ECA_Dodge);
		PlayAnimMontage(DodgeMontage);
	}
	else
	{
		if (GetCharacterAction() == ECharacterActions::ECA_Finish) return;
		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		SetCharacterAction(ECharacterActions::ECA_Dodge);
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

void APlayerMain::StartAttackBufferEvent(float BufferAmount)
{
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->PlayFromStart();
	}
}

void APlayerMain::StopDodgeBufferEvent()
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->Stop();
	}
}

void APlayerMain::StopAttackBufferEvent()
{
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->Stop();
	}
}

void APlayerMain::UpdateDodgeBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferDodgeDistance);
}

void APlayerMain::UpdateAttackBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferAttackDistance);
}

void APlayerMain::UpdateBuffer(float Alpha, float BufferDistance)
{
	FVector CurrentLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();

	FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	SetActorLocation(TargetLocation, true);
}


void APlayerMain::ResetLightAttackStats()
{
	LightAttackIndex = 0;
	IsSaveLightAttack = false;
}

void APlayerMain::ResetSpectralAttackStats()
{
	SpectralAttackIndex = 0;
	IsSaveLightAttack = false;
}

void APlayerMain::ResetJumpAttackStats()
{
	JumpAttackIndex = 0;
	isLaunched = false;
}

void APlayerMain::ResetHeavyAttackStats()
{
	HeavyAttackIndex = 0;
	IsSaveHeavyAttack = false;
}

void APlayerMain::SoftLockOn()
{
	FVector Start = GetActorLocation();
	FVector End = (GetLastMovementInputVector() * SoftLockDistance) + GetActorLocation();

	AActor* Enemy = SphereTraceForEnemies(Start, End);

	if (Enemy != Cast<ASpectre>(Enemy))
	{
		SoftLockTarget = Enemy;
		RotationToTarget();
	}
	else SoftLockTarget = nullptr;
}

void APlayerMain::RotationToTarget()
{
	if (SoftLockTarget && PlayerFormComponent && PlayerFormComponent->GetCharacterForm() != ECharacterForm::ECF_Spectral)
	{
		SoftLockTimeline->PlayFromStart();
	}
}

void APlayerMain::UpdateSoftLockOn(float Alpha)
{
	FVector Start = GetActorLocation();

	if (!SoftLockTarget) return;
	FVector End = SoftLockTarget->GetActorLocation();

	FRotator NewRotation = FRotator(
		GetActorRotation().Pitch,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
	);

	NewRotation = FMath::Lerp(GetActorRotation(), NewRotation, Alpha);

	SetActorRotation(NewRotation);
}

void APlayerMain::SearchTarget()
{
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() + GetViewRotation().Vector() * TrackTargetDistance;

	AActor* Enemy = SphereTraceForEnemies(Start, End);

	if (Enemy)
	{
		SpectralTarget = Cast<ASpectre>(Enemy);
	}
	else SpectralTarget = nullptr;
}

ECharacterActions APlayerMain::SetCharacterAction(ECharacterActions NewState)
{
	if (NewState != CharacterAction)
	{
		CharacterAction = NewState;
	}
	return NewState;
}

bool APlayerMain::IsActionEqualToAny(const TArray<ECharacterActions>& StatesToCheck)
{
	return StatesToCheck.Contains(CharacterAction);
}

bool APlayerMain::IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck)
{
	return StatesToCheck.Contains(CharacterState);
}

bool APlayerMain::IsFormEqualToAny(const TArray<ECharacterForm>& StatesToCheck)
{
	return StatesToCheck.Contains(PlayerFormComponent->GetCharacterForm());
}


float APlayerMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//TODO: player healthbar and fx to receive damage
	if (!bCanReceiveDamage) return 0.f;
	
	if (Attributes && Attributes->IsAlive())
	{
		Attributes->ReceiveDamage(DamageAmount);
		GetDirectionalReact();
	}
	//else
	//{
	//	Die();
	//}
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
		if (!TargetEnemy) return;

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
	else FinishEnemy();
}

void APlayerMain::ReleasePossession()
{
	if (PlayerControllerRef)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 1.f, FColor::Green, FString("There is a PC valid"));
		}

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
	if (GetCharacterAction() == ECharacterActions::ECA_Block || GetCharacterAction() == ECharacterActions::ECA_Finish) return;

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
	if (GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void APlayerMain::Jump()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Block || GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	PlayAnimMontage(JumpMontage, 1.f);

	Super::Jump();

	if (GetCharacterMovement()->IsFalling() && CanDoubleJump)
	{
		DoubleJump();
	}
}

void APlayerMain::DoubleJump()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Block) return;

	PlayAnimMontage(DoubleJumpMontage);
	LaunchCharacter(FVector(0.f, 0.f, 800.f), false, true);
	CanDoubleJump = false;
}

void APlayerMain::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	isLaunched = false;
	CanDoubleJump = true;
}

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (PlayerFormComponent->GetCharacterForm() == ECharacterForm::ECF_Human)
	{
		if (ASword* OverlappingWeapon = Cast<ASword>(OverlappingItem))
		{
			OverlappingWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);
			CharacterState = ECharacterStates::ECS_EquippedSword;
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
	//TODO: put the attack logic here, we have to translate it from BP_PlayerMain
}

void APlayerMain::ToggleForm() //TODO: extract to PlayerMain the "apply effects" functions for more flexibility here
{
	if (GetCharacterAction() == ECharacterActions::ECA_Dead 
		|| GetCharacterAction() == ECharacterActions::ECA_Block 
		|| GetCharacterAction() == ECharacterActions::ECA_Finish 
		|| GetCharacterAction() == ECharacterActions::ECA_Attack) return;

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
	}

	LastTransformationTime = CurrentTime;
}

void APlayerMain::WithEnergy()
{
	if (Attributes->ItHasEnergy())
	{
		PlayerFormComponent->ToggleForm(true);
		Attributes->RegenerateTick();
		Attributes->StartDecreaseEnergy();
		Attributes->OnDepletedCallback = [this]() { OutOfEnergy(); };
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
		SetCharacterAction(ECharacterActions::ECA_Dead);
	}
}

void APlayerMain::GetDirectionalReact()
{
	PlayAnimMontage(HitReactMontage);
}

void APlayerMain::Block()
{
	if (GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
		SetCharacterAction(ECharacterActions::ECA_Block);
	}	
} 

void APlayerMain::ReceiveBlock()
{
	PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		SetCharacterAction(ECharacterActions::ECA_Nothing);
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SprinEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::ReleaseBlock()
{
	StopAnimMontage(BlockMontage);
	SetCharacterAction(ECharacterActions::ECA_Nothing);
}

void APlayerMain::FinishEnemy()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	TArray<AActor*> Array;
	FinisherCollision->GetOverlappingActors(Array);

	for (AActor* Actor : Array)
	{
		if (Actor && Actor->GetClass()->ImplementsInterface(UHitInterface::StaticClass()))
		{
			if (IHitInterface::Execute_CanBeFinished(Actor))
			{
				bCanReceiveDamage = false;
				SetCharacterAction(ECharacterActions::ECA_Finish);
				FollowCamera->AttachToComponent(
					CameraFinisherLocation,
					FAttachmentTransformRules::SnapToTargetIncludingScale
				);
				PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
				PlayerControllerRef->DisableInput(PlayerControllerRef);

				IHitInterface::Execute_GetFinished(Actor, FinisherLocation->GetComponentLocation());
				PlayAnimMontage(FinisherMontage, 1.0f);

				FVector Start = GetActorLocation();
				FVector End = Actor->GetActorLocation();

				FRotator NewRotation = FRotator(
					GetActorRotation().Pitch,
					UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
					UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
				);

				SetActorRotation(NewRotation);
				Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
			}
			else return;
		}
	}
}

void APlayerMain::LaunchCharacterUp()
{
	isLaunched = true;
	AddActorWorldOffset(FVector(0.f, 0.f, 300.f), false);

	APaladin* Enemy = Cast<APaladin>(SoftLockTarget);
	if (Enemy)
	{
		Enemy->LaunchEnemyUp();
	}
}

void APlayerMain::Crasher()
{
	SoftLockOn();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	CharacterAction = ECharacterActions::ECA_Attack;

	FVector Start = GetActorLocation();
	FVector End = Start + FVector(0.f, 0.f, -100000.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(this);

	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::LineTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		ObjectTypes,
		false,
		ObjectsToIgnore,
		EDrawDebugTrace::None,
		Hit,
		true
	);

	if (bHit)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		SetActorLocation(Hit.ImpactPoint);
	}
}

AActor* APlayerMain::SphereTraceForEnemies(FVector Start, FVector End)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		TrackTargetRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		ResultHit,
		true
	);

	return ResultHit.GetActor();
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
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &APlayerMain::Block);
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &APlayerMain::ReleaseBlock);
		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Completed, this, &APlayerMain::PossessEnemy);
		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Completed, this, &APlayerMain::RestartLevel);
		EnhancedInputComponent->BindAction(GoToMenuAction, ETriggerEvent::Completed, this, &APlayerMain::GoToMainMenu);
	}
}