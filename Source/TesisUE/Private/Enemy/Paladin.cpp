#include "Enemy/Paladin.h"
#include "Player/PlayerMain.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/DamageTypeMain.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/PossessionComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Interfaces/HitInterface.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include <Kismet/KismetMathLibrary.h>


APaladin::APaladin()
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);

	GetAttributeComponent()->AttachShield(GetMesh(), FName("LeftHandSocket"));

	DefaultDamage = Damage;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
	SwordMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SwordCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordCollider->SetupAttachment(SwordMesh);
	SwordCollider->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SwordCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordCollider->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(SwordMesh);

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(SwordMesh);

	SwordCollider->OnComponentBeginOverlap.AddDynamic(this, &APaladin::OnSwordOverlap);

	OnShieldTakeDamage.AddDynamic(this, &APaladin::ShieldHit);
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();

	if (!BBComponent)
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
	}
}

void APaladin::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	Super::ActivateEnemy(Location, Rotation);

	if (GetCharacterStateComponent())
	{
		GetCharacterStateComponent()->SetHumanState(ECharacterHumanStates::ECHS_EquippedSword);
	}

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APaladin::DeactivateEnemy()
{
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::DeactivateEnemy();
}

bool APaladin::IsLaunchable()
{
	return !GetAttributeComponent()->IsShielded(); //returns false if it has shield equipped or not detached yet.
}

void APaladin::LaunchUp(const FVector& InstigatorLocation)
{
	Super::LaunchUp(InstigatorLocation);

	LaunchEnemyUp(InstigatorLocation);
}

void APaladin::Die(UAnimMontage* DeathAnim, const FName Section)
{
	Super::Die(DeathAnim, Section);

	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying || GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		LaunchCharacter(FVector(0.f, 0.f, -300.f), true, true);
	}
	
	NotifyDamageTakenToBlackboard(LastDamageCauser);
}

void APaladin::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	GetAttributeComponent()->DecreaseEnergyBy(PossessionAttackCost);

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
	
	IgnoreActors.Add(this);

	const bool bHitOccurred = UKismetSystemLibrary::BoxTraceMulti(
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
			if (IHitInterface* Entity = Cast<IHitInterface>(Hit.GetActor()))
			{
				UGameplayStatics::ApplyDamage(
					Hit.GetActor(),
					Damage,
					GetController(),
					this,
					UDamageType::StaticClass()
				);

				FDamageEvent DamageEvent(UDamageType::StaticClass());
				
				AEntity* FinalDamageCauser = nullptr;	
				if (GetPossessionComponent()->IsPossessed()) FinalDamageCauser =  this;
				
				Entity->GetHit(FinalDamageCauser, Hit.ImpactPoint, DamageEvent, Damage);
				
				PlayCameraShake(SwordMesh->GetComponentLocation(), 0.f, 500.f);
				IgnoreActors.Add(Hit.GetActor());
			}
		}
	}
}

void APaladin::ApplyPossessionParameters(const bool bShouldEnable)
{
	Super::ApplyPossessionParameters(bShouldEnable);
	if (bShouldEnable)
	{
		Damage = PossessionDamage;
		GetCharacterMovement()->MaxWalkSpeed = PossessionMaxWalkSpeed;
	}
	else
	{
		Damage = DefaultDamage;
		GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	}
}

void APaladin::ShieldHit()
{
	if (HitReactMontage)
	{
		StopAnimMontage();
		PlayAnimMontage(HitReactMontage, 1.f, FName("ShieldHit"));
	}
}

void APaladin::LaunchEnemyUp(const FVector& InstigatorLocation)
{
	if (bIsLaunched) return;

	bIsLaunched = true;
	DisableAI();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
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
	SetActorLocation(FVector(GetTargetActorLocation().X, GetTargetActorLocation().Y, PlayerLocationZ));
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
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
		FinishedDamage();
		break;

	case EMainDamageTypes::EMDT_Slash:
		Slash();
		break;

	case EMainDamageTypes::EMDT_Puncture:
		PlayAnimMontage(HitReactMontage, 1.f, FName("PunctureReact"));
		break;

	case EMainDamageTypes::EMDT_Impact:
		PlayAnimMontage(HitReactMontage, 1.f, FName("ImpactReact"));
		break;

	case EMainDamageTypes::EMDT_None:
		GetDirectionalReact(ImpactPoint);
		break;

	default:
		GetDirectionalReact(ImpactPoint);
		break;
	}
}

void APaladin::Slash()
{
	FRotator DamageCauserLocation = UKismetMathLibrary::FindLookAtRotation(GetTargetActorLocation(), LastDamageCauser->GetTargetActorLocation());

	SetActorRotation(FRotator(0.f, DamageCauserLocation.Yaw, 0.f));

	StopAnimMontage();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromFrontBig"));
}
