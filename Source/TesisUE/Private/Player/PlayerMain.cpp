// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerMain.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/PlayerFormComponent.h"


APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());

	MainCam = CreateDefaultSubobject<UCameraComponent>(TEXT("Player Camera"));
	MainCam->SetupAttachment(CameraBoom);

	BufferDodgeTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferDodgeTimeline"));
	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));

	PlayerFormComponent = CreateDefaultSubobject<UPlayerFormComponent>(TEXT("PlayerFormComponent"));

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
	//UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	//if (AnimInstance)
	
		if (GetCharacterAction() != ECharacterActions::ECA_Attack)
		{
			StopAttackBufferEvent();
			AttackBufferEvent(BufferAttackDistance);
			AttackMontage = LightAttackCombo[AttackIndex];
			SetCharacterState(ECharacterActions::ECA_Attack);

			SoftLockOn();

			//AnimInstance->Montage_Play(AttackMontage);
			PlayAnimMontage(AttackMontage);
			LightAttackIndex++;

			if (LightAttackIndex >= LightAttackCombo.Num())
			{
				LightAttackIndex = 0;
			}
		}
	
}

void APlayerMain::PerformHeavyAttack(int AttackIndex)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (GetCharacterAction() != ECharacterActions::ECA_Attack)
		{
			StopAttackBufferEvent();
			AttackBufferEvent(BufferAttackDistance);
			HeavyAttackMontage = HeavyAttackCombo[AttackIndex];
			SetCharacterState(ECharacterActions::ECA_Attack);

			SoftLockOn();

			AnimInstance->Montage_Play(HeavyAttackMontage);
			HeavyAttackIndex++;

			if (HeavyAttackIndex >= HeavyAttackCombo.Num())
			{
				HeavyAttackIndex = 0;
			}
		}
	}
}

void APlayerMain::PerformDodge()
{
	StopDodgeBufferEvent();
	DodgeBufferEvent(BufferDodgeDistance);
	SetCharacterState(ECharacterActions::ECA_Dodge);
	PlayAnimMontage(DodgeMontage);
}


void APlayerMain::DodgeBufferEvent(float BufferAmount)
{
	if (BufferDodgeTimeline)
	{
		BufferDodgeTimeline->PlayFromStart();
	}
}

void APlayerMain::AttackBufferEvent(float BufferAmount)
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
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	FHitResult ResultHit;

	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		GetActorLocation(),
		(GetLastMovementInputVector() * SoftLockDistance) + GetActorLocation(),
		SoftLockRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		ResultHit,
		true
		);

	if (ResultHit.GetActor())
	{
		SoftLockTarget = ResultHit.GetActor();
	}
	else SoftLockTarget = nullptr;
}

void APlayerMain::RotationToTarget()
{
	if (SoftLockTarget)
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


void APlayerMain::Move(const FInputActionValue& Value)
{
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

void APlayerMain::Interact(const FInputActionValue& Value)
{
	if (ASword* OverlappingWeapon = Cast<ASword>(OverlappingItem))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Green, FString("Overlapping"));
		}

		OverlappingWeapon->Equip(GetMesh(), FName("RightHandSocket"), this, this);

		CharacterState = ECharacterStates::ECS_EquippedSword;
		OverlappingItem = nullptr;
		EquippedWeapon = OverlappingWeapon;
	}
}

void APlayerMain::Attack(const FInputActionValue& Value)
{
	//TODO: pasar la logica aca en esta funcion
}

void APlayerMain::ToggleForm()
{
	if (PlayerFormComponent)
	{
		PlayerFormComponent->ToggleForm(EquippedWeapon);
	}
}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerMain::Interact);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
		EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &APlayerMain::ToggleForm);
	}
}
