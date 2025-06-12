#include "Enemy/Paladin.h"
#include "Player/PlayerMain.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/DamageTypeMain.h"
#include "DamageTypes/SpectralTrapDamageType.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CombatComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Interfaces/HitInterface.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include <Player/PlayerHeroController.h>


APaladin::APaladin()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);

	Attributes->AttachShield(GetMesh(), FName("LeftHandSocket"));

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
	SwordMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SwordCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordCollider->SetupAttachment(SwordMesh);
	
	SwordCollider->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	if (SwordCollider)
	{
		SwordCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SwordCollider->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(SwordMesh);
	
	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(SwordMesh);
}

bool APaladin::IsLaunchable_Implementation(ACharacter* DamageCauser)
{
	return !Attributes->IsShielded();
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();
	
	SwordCollider->OnComponentBeginOverlap.AddDynamic(this, &APaladin::OnSwordOverlap);

	if (Attributes)
	{
		Attributes->OnDettachShield.AddLambda(
			[this]
			{
				if (AAIController* AIController = Cast<AAIController>(GetController()))
				{
					if (UBlackboardComponent* BBComponent = AIController->GetBlackboardComponent())
					{
						BBComponent->SetValueAsBool(FName("IsShielded"), false);
					}
				}
			}
		);

		//pongo un lambda porque si pongo la funcion die me va a pedir el dmg causer jeje
		Attributes->OnEntityDead.AddLambda(
			[this]
			{
				Die(DamageCauserOf);
			}
		);
	}
}

void APaladin::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	Super::ActivateEnemy(Location, Rotation);

	if (CharacterStateComponent)
	{
		CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword); // Set Paladin's default active state
	}

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APaladin::DeactivateEnemy()
{
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::DeactivateEnemy();
}

void APaladin::Die(AActor* DamageCauser)
{
	Super::Die(DamageCauser);

	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying || GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		LaunchCharacter(FVector(0.f, 0.f, -300.f), true, true);
	}
	
	TArray<AEnemy*> NearbyEnemies = GenerateSphereOverlapToDetectOtherEnemies(GetActorLocation(), this);
	for (AEnemy* NearbyEnemy : NearbyEnemies)
	{
		if (IsValid(NearbyEnemy))
		{
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
				AIController->GetBlackboardComponent()->ClearValue(FName("CanSeePlayer"));
			}
		}
	}
}

void APaladin::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (SwordCollider)
	{
		SwordCollider->SetCollisionEnabled(CollisionEnabled);
		IgnoreActors.Empty();
	}
}

void APaladin::OnSwordOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	TArray<FHitResult> HitResults;
	//FHitResult BoxHit;
	
	IgnoreActors;
	IgnoreActors.Add(this);

	bool bHitOccurred = UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(10.f, 10.f, 10.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		if (bHitOccurred && Hit.GetActor() && !IgnoreActors.Contains(Hit.GetActor()))
		{
			IHitInterface* Entity = Cast<IHitInterface>(Hit.GetActor());
			if (Entity)
			{
				UGameplayStatics::ApplyDamage(
					Hit.GetActor(),
					Damage,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
				);

				Entity->Execute_GetHit(Hit.GetActor(), Hit.ImpactPoint, UDamageType::StaticClass());
				UGameplayStatics::PlayWorldCameraShake(this, CameraShake, SwordMesh->GetComponentLocation(), 0.f, 500.f);
				IgnoreActors.Add(Hit.GetActor());
			}
		}
	}
}

void APaladin::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	EnhancedInputComponent->BindAction(CombatComponent->HeavyAttackAction, ETriggerEvent::Triggered, this, &APaladin::HeavyAttack);
}

void APaladin::GetDefaultParameters()
{
	Super::GetDefaultParameters();

	DefaultDamage = Damage;
}

void APaladin::SetOnPossessedParameters()
{
	Super::SetOnPossessedParameters();

	Damage = PossessionDamage;
}

void APaladin::ShieldHit()
{
	if (HitReactMontage)
	{
		PlayAnimMontage(HitReactMontage, 1.f, FName("ShieldHit"));
	}
}

void APaladin::Attack(const FInputActionValue& Value)
{
	Super::Attack(Value);

	Attributes->DecreaseEnergyBy(PossessionAttackCost);

	CombatComponent->Input_Attack(Value);
}

void APaladin::HeavyAttack(const FInputActionValue& Value)
{
	Attributes->DecreaseEnergyBy(PossessionHeavyAttackCost);

	CombatComponent->Input_HeavyAttack(Value);
}



void APaladin::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
	if (!Attributes->IsShielded()) LaunchEnemyUp(InstigatorLocation);
}

void APaladin::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType)
{
	if (!Attributes || GetEnemyState() == EEnemyState::EES_Died) return;
	
	if (Attributes->IsShielded() && DamageType == USpectralTrapDamageType::StaticClass())
	{
		StopAnimMontage();
		ShieldHit();
	}

	Super::GetHit_Implementation(ImpactPoint, DamageType); //sfx and fx

	//else if (Attributes->IsAlive())
	//{
	//	Super::GetHit_Implementation(ImpactPoint, DamageType); //sfx and fx
	//	ReactToDamage(LastDamageType, ImpactPoint);
	//}
}

float APaladin::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes->IsAlive())
	{
		if (Attributes->IsShielded() && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
		{
			NotifyDamageTakenToBlackboard(DamageCauser);
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Attributes->GetShieldMeshComponent()->GetComponentLocation());
			Attributes->ReceiveShieldDamage(DamageAmount);
		}
		else if (!Attributes->IsShielded() && DamageEvent.DamageTypeClass != USpectralTrapDamageType::StaticClass())
		{
			Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		}
	}
	return DamageAmount;
}

void APaladin::LaunchEnemyUp(const FVector& InstigatorLocation)
{
	if (isLaunched) return;

	isLaunched = true;
	DisableAI();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, InstigatorLocation.Z));
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void APaladin::CrashDown()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	PlayAnimMontage(HitReactMontage, 1.f, FName("KnockDown"));
	LaunchCharacter(FVector(0.f, 0.f, -100000.f), true, true);
}

void APaladin::HitInAir()
{
	float PlayerLocationZ = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation().Z;
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, PlayerLocationZ));
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	GetCharacterMovement()->IsFlying();
	DisableAI();
}

void APaladin::ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint)
{
	switch (DamageType)
	{
	case EMainDamageTypes::EMDT_CrashDown:
		CrashDown();
		break;

	case EMainDamageTypes::EMDT_InAir:
		HitInAir();
		break;

	case EMainDamageTypes::EMDT_Finisher:
		GetFinished_Implementation();
		return;
		break;
	default:
		Super::ReactToDamage(DamageType, ImpactPoint);
		break;
	}
}