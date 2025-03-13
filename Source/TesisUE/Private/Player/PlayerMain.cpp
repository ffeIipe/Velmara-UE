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

	BufferTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferTimeline"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void APlayerMain::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = CastChecked<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			Subsystem->AddMappingContext(CharacterContext, 0);

	if (BufferCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("UpdateBuffer"));
		BufferTimeline->AddInterpFloat(BufferCurve, ProgressFunction);
	}
}

void APlayerMain::PerformLightAttack(int AttackIndex)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (GetCharacterState() != ECharacterStates::ECS_Attack)
		{
			AttackMontage = LightAttackCombo[AttackIndex];
			SetCharacterState(ECharacterStates::ECS_Attack);
			AnimInstance->Montage_Play(AttackMontage);
			LightAttackIndex++;

			if (LightAttackIndex >= LightAttackCombo.Num())
			{
				LightAttackIndex = 0;
			}
		}
	}
}

void APlayerMain::PerformHeavyAttack(int AttackIndex)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (GetCharacterState() != ECharacterStates::ECS_Attack)
		{
			StopBufferEvent();
			BufferEvent(BufferDistance);
			HeavyAttackMontage = HeavyAttackCombo[AttackIndex];
			SetCharacterState(ECharacterStates::ECS_Attack);
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
	StopBufferEvent();
	BufferEvent(BufferDistance);
	SetCharacterState(ECharacterStates::ECS_Dodge);
	PlayAnimMontage(DodgeMontage);
}

void APlayerMain::BufferEvent(float BufferAmount)
{
	if (BufferTimeline)
	{
		BufferTimeline->PlayFromStart();
	}
}

void APlayerMain::StopBufferEvent()
{
	if (BufferTimeline)
	{
		BufferTimeline->Stop();
	}
}

void APlayerMain::UpdateBuffer(float Alpha)
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

ECharacterStates APlayerMain::SetCharacterState(ECharacterStates NewState)
{
	if (NewState != CharacterState)
	{
		CharacterState = NewState;
	}
	return NewState;
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

void APlayerMain::Attack(const FInputActionValue& Value)
{
	
}

void APlayerMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlayerMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerMain::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerMain::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerMain::Jump);
		//EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &APlayerMain::Interact);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APlayerMain::Attack);
	}
}

