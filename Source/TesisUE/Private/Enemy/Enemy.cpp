// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AttributeComponent.h"
#include "HUD/HealthBarComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Player/PlayerMain.h"
#include "DamageTypes/DamageTypeMain.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	SpringArm->SocketOffset = FVector(0.f,0.f,45.f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

	HealthBarWidget = CreateDefaultSubobject<UHealthBarComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());
	HealthBarWidget->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

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
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIOriginalController = AIController;
	}

	if (GetMesh())
	{
		DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	}
}

void AEnemy::Die()
{
	const int32 RandomValue = FMath::RandRange(0, 3);
	FName SectionName = FName();

	switch (RandomValue)
	{
	case 0:
		SectionName = FName("Death1");
		break;
	case 1:
		SectionName = FName("Death2");
		break;
	case 2:
		SectionName = FName("Death3");
		break;
	case 3:
		SectionName = FName("Death4");
		break;
	default:
		break;
	}

	PlayAnimMontage(DeathMontage, 1.f, SectionName);

	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	HealthBarWidget->SetHealthBarActive(false);

	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->StopMovement();  
		AIController->UnPossess();   
	}

	SetLifeSpan(7.f);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEnemy::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEnemy::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AEnemy::Jump);
		EnhancedInputComponent->BindAction(UnPossessAction, ETriggerEvent::Completed, this, &AEnemy::UnPossess);
	}
}

void AEnemy::Move(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(DirectionForward, MoveVector.Y);
	AddMovementInput(DirectionSideward, MoveVector.X);
}

void AEnemy::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void AEnemy::LaunchEnemyUp()
{
	if (isLaunched) return;

	isLaunched = true;
	DisableAI();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	AddActorWorldOffset(FVector(0.f,0.f,400.f), true);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void AEnemy::CrashDown()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	PlayAnimMontage(HitReactMontage, 1.f, FName("KnockDown"));
	LaunchCharacter(FVector(0.f, 0.f, -100000.f), true, true);
}

void AEnemy::HitInAir()
{
	float PlayerLocationZ = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation().Z;
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, PlayerLocationZ + 50.f));
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	GetCharacterMovement()->IsFlying();
	DisableAI();

}

void AEnemy::ResetEnemy()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	isLaunched = false;
	EnableAI();
}

void AEnemy::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (Attributes && Attributes->IsAlive())
	{
		//DirectionalHitReact(ImpactPoint);
		HitFlash();
	}
	else
	{
		Die();
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint);
	}
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitParticles,
			ImpactPoint
		);
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (Attributes && Attributes->IsAlive() && HealthBarWidget)
	{
		Attributes->ReceiveDamage(DamageAmount);
		HealthBarWidget->SetHealthPercent(Attributes->GetHealthPercent());
	}
	return DamageAmount;
}

void AEnemy::ReceiveAnyDamage(float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	Super::ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser);

	//const UDamageTypeMain* MainDamageType = Cast<UDamageTypeMain>(DamageType);
	//if (MainDamageType)
	//{
	//	switch (MainDamageType->DamageType)
	//	{
	//	case EMainDamageTypes::CrashDown:
	//		UE_LOG(LogTemp, Warning, TEXT("Received CrashDown damage"));
	//		// Tu lógica para este tipo
	//		break;
	//
	//	case EMainDamageTypes::InAir:
	//		UE_LOG(LogTemp, Warning, TEXT("Received InAir damage"));
	//		// Otra lógica
	//		break;
	//
	//	default:
	//		UE_LOG(LogTemp, Warning, TEXT("Unknown damage type"));
	//		break;
	//	}
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("DamageType is not of type UDamageTypeMain"));
	//}
}

void AEnemy::DirectionalHitReact(const FVector& ImpactPoint)
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

	if (Angle >= -45.f && Angle < 45.f)
	{
		Section = FName("FromFront");
	}

	else if (Angle >= -135.f && Angle < -45.f)
	{
		Section = FName("FromLeft");
	}

	else if (Angle >= 45.f && Angle < 135.f)
	{
		Section = FName("FromRight");
	}

	PlayAnimMontage(HitReactMontage, 1.f, Section);
}

void AEnemy::HitFlash()
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("HitFlashAmount"), 0.0f);
		GetWorldTimerManager().SetTimer(HitFlashTimerHandle, this, &AEnemy::ResetColor, 0.2f, false);
	}
}

void AEnemy::ResetColor()
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("HitFlashAmount"), 1.0f);
	}
}

void AEnemy::DisableAI()
{
	if (AIOriginalController)
	{
		AIOriginalController->StopMovement();
		AIOriginalController->UnPossess();
	}
}

void AEnemy::EnableAI()
{
	if (AIOriginalController)
	{
		AIOriginalController->Possess(this);
	}
}

USpringArmComponent* AEnemy::GetSpringArm()
{
	return SpringArm;
}

void AEnemy::OnPossessed(APlayerMain* NewOwner)
{
	PossessionOwner = NewOwner;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 800.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;
}

void AEnemy::UnPossess()
{
	bUseControllerRotationYaw = true;
	PossessionOwner->ReleasePossession();
}

