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
#include <Enemy/Spectre.h>
#include "Camera/CameraActor.h"
#include "EngineUtils.h"

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
	if (GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		if (PlayerFormComponent->GetCurrentForm() == EPlayerForm::EPF_Human)
		{
			StopAttackBufferEvent();
			StartAttackBufferEvent(BufferAttackDistance);
			SetCharacterState(ECharacterActions::ECA_Attack);
			SoftLockOn();
			PlayAnimMontage(LightAttackCombo[AttackIndex]);
		}
		else
		{
			SetCharacterState(ECharacterActions::ECA_Attack);
			SearchTarget();
			PlayAnimMontage(SpectralAttackCombo[AttackIndex]);
		}

		LightAttackIndex++;

		if (LightAttackIndex >= LightAttackCombo.Num())
		{
			LightAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformHeavyAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack)
	{
		if (PlayerFormComponent->GetCurrentForm() == EPlayerForm::EPF_Human)
		{
			StopAttackBufferEvent();
			StartAttackBufferEvent(BufferAttackDistance);
			SetCharacterState(ECharacterActions::ECA_Attack);

			PlayAnimMontage(HeavyAttackCombo[AttackIndex]);

			SoftLockOn();
		}
		else
		{
			SetCharacterState(ECharacterActions::ECA_Attack);
			PlayAnimMontage(SpectralHeavyAttack);
		}

		HeavyAttackIndex++;

		if (HeavyAttackIndex >= HeavyAttackCombo.Num())
		{
			HeavyAttackIndex = 0;
		}
	}
}

void APlayerMain::PerformDodge()
{
	if (PlayerFormComponent && PlayerFormComponent->GetCurrentForm() == EPlayerForm::EPF_Human)
	{
		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		SetCharacterState(ECharacterActions::ECA_Dodge);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		PlayAnimMontage(DodgeMontage);
	}
	else
	{
		StopDodgeBufferEvent();
		DodgeBufferEvent(BufferDodgeDistance);
		SetCharacterState(ECharacterActions::ECA_Dodge);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
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
	}
	else SoftLockTarget = nullptr;
}

void APlayerMain::RotationToTarget()
{
	if (SoftLockTarget && PlayerFormComponent && PlayerFormComponent->GetCurrentForm() != EPlayerForm::EPF_Spectral)
	{
		SoftLockTimeline->PlayFromStart();
	}
}

void APlayerMain::UpdateSoftLockOn(float Alpha)
{
	FVector Start = GetActorLocation();
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

ECharacterActions APlayerMain::SetCharacterState(ECharacterActions NewState)
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

bool APlayerMain::IsFormEqualToAny(const TArray<EPlayerForm>& StatesToCheck)
{
	return StatesToCheck.Contains(PlayerFormComponent->GetCurrentForm());
}


float APlayerMain::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	//TODO: player healthbar and fx to receive damage

	if (Attributes && Attributes->IsAlive())
	{
		if (GetCharacterAction() == ECharacterActions::ECA_Block)
		{
			ReceiveBlock();
		}
		else
		{
			Attributes->ReceiveDamage(DamageAmount);
			GetDirectionalReact();
		}
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
	FVector Start = GetActorLocation() + GetActorForwardVector() * 100.0f;
	FVector End = Start + GetActorForwardVector() * 1000.0f;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);
		return Cast<AEnemy>(HitResult.GetActor());
	}
	return nullptr;
}

void APlayerMain::PossessEnemy()
{
	if (bIsPossessing) ReleasePossession();

	else
	{
		AEnemy* TargetEnemy = GetTargetEnemy();
		if (!TargetEnemy) return;

		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			PC->Possess(TargetEnemy);
			TargetEnemy->DisableAI();
			PossessedEnemy = TargetEnemy;

			TargetEnemy->EnableInput(PC);
			TargetEnemy->AutoPossessPlayer = EAutoReceiveInput::Player0;
			TargetEnemy->OnPossessed(this);

			FollowCamera->AttachToComponent(TargetEnemy->SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
			PC->SetViewTargetWithBlend(FollowCamera, 1.f);
		}

		bIsPossessing = true;
		StoredLocation = GetActorLocation();
		StoredRotation = GetActorRotation();
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}	
}

void APlayerMain::ReleasePossession()
{
	if (!PossessedEnemy) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->Possess(this);
		PossessedEnemy->DisableInput(PC);
		PossessedEnemy->EnableAI();
		PossessedEnemy = nullptr;
		FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
		PC->SetViewTargetWithBlend(FollowCamera, 1.f);
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorLocation(StoredLocation);
	SetActorRotation(StoredRotation);

	//if (MainCam)
	//{
	//	MainCam->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	//}
}

void APlayerMain::Move(const FInputActionValue& Value)
{
	if (GetCharacterAction() == ECharacterActions::ECA_Block) return;

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
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void APlayerMain::Jump()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Block) return;

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
	CanDoubleJump = true;
}

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (PlayerFormComponent->GetCurrentForm() == EPlayerForm::EPF_Human)
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

void APlayerMain::ToggleForm()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Block) return;

	if (PlayerFormComponent)
	{
		PlayerFormComponent->ToggleForm(EquippedWeapon);
	}
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
		SetCharacterState(ECharacterActions::ECA_Dead);
	}
}

void APlayerMain::GetDirectionalReact()
{
	PlayAnimMontage(HitReactMontage);
}

void APlayerMain::Block()
{
	PlayAnimMontage(BlockMontage,1.f, FName("BlockIdle"));
	SetCharacterState(ECharacterActions::ECA_Block);
}

void APlayerMain::ReceiveBlock()
{
	PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
}

void APlayerMain::ReleaseBlock()
{
	StopAnimMontage(BlockMontage);
	SetCharacterState(ECharacterActions::ECA_Nothing);
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
		EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Triggered, this, &APlayerMain::PossessEnemy);
	}
}