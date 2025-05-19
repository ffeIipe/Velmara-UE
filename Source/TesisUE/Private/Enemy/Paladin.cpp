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

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Interfaces/HitInterface.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "AI/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"


APaladin::APaladin()
{
	Attributes->AttachShield(GetMesh(), FName("LeftHandSocket"));

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
	SwordMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	SwordCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordCollider->SetupAttachment(SwordMesh);
	SwordCollider->SetRelativeLocation(FVector(0.f, 0.f, 48.f));
	SwordCollider->SetBoxExtent(FVector(3.f, 2.f, 36.f));
	SwordCollider->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 11.f));

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 82.f));

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterStateComponent"));
}

bool APaladin::IsLaunchable_Implementation(ACharacter* DamageCauser)
{
	if (Attributes->IsShielded())
	{
		APlayerMain* TempPlayerRef = Cast<APlayerMain>(DamageCauser);
		TempPlayerRef->CombatComponent->HitReactJumpToSection(FName("ReactToShield"));
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Attributes->GetShieldMeshComponent()->GetComponentLocation());

		return false; //its not launchable
	}
	else
	{
		return true; //now its launchable, because it has no longer the shield equipped
	}
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();
	
	SwordCollider->OnComponentBeginOverlap.AddDynamic(this, &APaladin::OnSwordOverlap);

	CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword);
}

void APaladin::Die(AActor* DamageCauser)
{
	Super::Die(DamageCauser);

	TArray<AEnemy*> NearbyEnemies = GenerateSphereOverlapToDetectOtherEnemies(GetActorLocation(), this);
	for (AEnemy* NearbyEnemy : NearbyEnemies)
	{
		if (IsValid(NearbyEnemy))
		{
			AAIController* AIController = Cast<AAIController>(NearbyEnemy->GetController());
			if (AIController)
			{
				AIController->GetBlackboardComponent()->ClearValue(FName("TargetActor"));
				Cast<AEnemyAIController>(AIController)->bPauseEnemyPerceptionUpdate = false;
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

	FHitResult BoxHit;
	
	IgnoreActors;
	IgnoreActors.Add(this);

	bool bHitOccurred = UKismetSystemLibrary::BoxTraceSingle(
		this,
		Start,
		End,
		FVector(10.f, 10.f, 10.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		IgnoreActors,
		EDrawDebugTrace::ForDuration,
		BoxHit,
		true
	);

	if (bHitOccurred && BoxHit.GetActor())
	{
		IHitInterface* Entity = Cast<IHitInterface>(BoxHit.GetActor());
		if (Entity)
		{
			UGameplayStatics::ApplyDamage(
				BoxHit.GetActor(),
				Damage,
				GetInstigatorController(),
				this,
				UDamageType::StaticClass()
			);

			Entity->Execute_GetHit(BoxHit.GetActor(), BoxHit.ImpactPoint, UDamageType::StaticClass());
			UGameplayStatics::PlayWorldCameraShake(this, CameraShake, SwordMesh->GetComponentLocation(), 0.f, 500.f);
		}

		if (PossessionOwner)
		{
			TArray<AEnemy*> NearbyEnemies = GenerateSphereOverlapToDetectOtherEnemies(BoxHit.ImpactPoint, BoxHit.GetActor());
			for (AEnemy* EnemyToAlert : NearbyEnemies)
			{
				if (IsValid(EnemyToAlert))
				{
					EnemyToAlert->NotifyThreat(this);
				}
			}
		}
	}
}

TArray<AEnemy*> APaladin::GenerateSphereOverlapToDetectOtherEnemies(const FVector& Origin, AActor* HitEnemyToExclude)
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

void APaladin::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	EnhancedInputComponent->BindAction(CombatComponent->AttackAction, ETriggerEvent::Triggered, this, &APaladin::Attack);
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
	CombatComponent->Input_Attack(Value);

}

void APaladin::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
	if (!Attributes->IsShielded()) LaunchEnemyUp(InstigatorLocation);
}

void APaladin::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType)
{
	Super::GetHit_Implementation(ImpactPoint, DamageType);

	if (Attributes && Attributes->IsAlive() && GetEnemyState() != EEnemyState::EES_Died)
	{
		if (DamageType == USpectralTrapDamageType::StaticClass()) return;

		ReactToDamage(LastDamageType, ImpactPoint);
	}
	else
	{
		PlayAnimMontage(DeathMontage, 1.f, SelectRandomDieAnim());
		Die(DamageCauserOf);
	}
}

UCharacterStateComponent* APaladin::GetCharacterStateComponent_Implementation()
{
	return CharacterStateComponent;
}

float APaladin::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes->IsAlive())
	{
		if (DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
		{
			if (Attributes->IsShielded())
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Attributes->GetShieldMeshComponent()->GetComponentLocation());
				Attributes->ReceiveShieldDamage(DamageAmount);
				ShieldHit();
			}
			else
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldDettachSFX, Attributes->GetShieldMeshComponent()->GetComponentLocation());
			}
		}
		else
		{
			if (!Attributes->IsShielded())
			{
				Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
			}
			else if (APlayerMain* TempPlayerRef = Cast<APlayerMain>(DamageCauser))
			{
				TempPlayerRef->CombatComponent->HitReactJumpToSection(FName("ReactToShield"));
				
				if (ShieldImpactSFX)
				{
					UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Attributes->GetShieldMeshComponent()->GetComponentLocation());
				}
			}
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
	Super::ReactToDamage(DamageType, ImpactPoint);

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
		DirectionalHitReact(ImpactPoint);
		break;
	}
}

void APaladin::UnPossessBase()
{
	Super::UnPossessBase();

	//TArray<AEnemy*> NearbyEnemies = GenerateSphereOverlapToDetectOtherEnemies(GetActorLocation(), this);
	//for (AEnemy* EnemyToAlert : NearbyEnemies)
	//{
	//	if (IsValid(EnemyToAlert))
	//	{
	//		EnemyToAlert->NotifyThreat(this);
	//	}
	//}
}
