#include "Enemy/Paladin/PaladinBoss.h"
#include <AI/EnemyAIController.h>
#include "BehaviorTree/BlackboardComponent.h"

#include "Components/AttributeComponent.h"
#include "Components/CombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpawnPointComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "SpectralTrapComponent.h"
#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include <Kismet/KismetMathLibrary.h>

#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>


APaladinBoss::APaladinBoss()
{
	SpectralTrapComponent = CreateDefaultSubobject<USpectralTrapComponent>(TEXT("SpectralTrapComponent"));
	SpectralTrapComponent->SphereCollider->SetupAttachment(Attributes->GetShieldMeshComponent());
}

void APaladinBoss::BeginPlay()
{
	Super::BeginPlay();

	GetComponents<USpawnPointComponent>(SpawnPoints);

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
						GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::White, FString("IS NOT SHIELDED ANYMORE"));
					}
				}
			}
		);
	}

	if (MinionToSpawnClass && InitialMinionPoolSize > 0)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>();
			if (PoolManager)
			{
				PoolManager->EnsurePoolInitialized(MinionToSpawnClass, InitialMinionPoolSize);
			}
		}
	}
}

float APaladinBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	return DamageAmount;
}

void APaladinBoss::DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage, const float DamageReceived)
{
	if (UKismetMathLibrary::RandomBool())
	{
		StopAnimMontage();
		PlayAnimMontage(HitReactMontage, 1.f, FName("FromRight"));
	}
	else
	{
		StopAnimMontage();
		PlayAnimMontage(HitReactMontage, 1.f, FName("FromLeft"));
	}
}

bool APaladinBoss::IsLaunchable_Implementation(ACharacter* DamageCauser)
{
	return Super::IsLaunchable_Implementation(DamageCauser);
}

void APaladinBoss::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, FString("GET HIT PALADIN BOSS"));

	NotifyDamageTakenToBlackboard(DamageCauserOf);
}

void APaladinBoss::TryToInvoke()
{
	if (CanInvoke())
	{
		Invoke();
	}
}

bool APaladinBoss::CanInvoke()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, FString::SanitizeFloat(Minions.Num()));
	return Minions.Num() <= 0;
}

void APaladinBoss::Invoke()
{
	UWorld* World = GetWorld();
	if (!World || !MinionToSpawnClass) return;

	UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>();
	if (!PoolManager) return;

	for (USceneComponent* SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint)
		{
			FVector SpawnLocation = SpawnPoint->GetComponentLocation();
			FRotator SpawnRotation = SpawnPoint->GetComponentRotation();

			AEnemy* EnemyFromPool = PoolManager->SpawnEnemyFromPool(MinionToSpawnClass, SpawnLocation, SpawnRotation, this, this);

			if (AEnemy* NewEnemy = Cast<AEnemy>(EnemyFromPool))
			{
				NewEnemy->OnDeactivated.AddDynamic(this, &APaladinBoss::HandleMinionDeactivated);

				Minions.Add(NewEnemy);
			}
			else if (EnemyFromPool)
			{
				PoolManager->ReturnEnemyToPool(EnemyFromPool);
			}
		}
	}
}

void APaladinBoss::FloodAttack()
{
	if (FloodToRaise)
	{
		StopAnimMontage();
		PlayAnimMontage(InvokeMontage);

		RaiseFlood();
	}
}

void APaladinBoss::HandleMinionDeactivated(AEnemy* DeactivatedMinion)
{
	if (AEnemy* EnemyToRemove = Cast<AEnemy>(DeactivatedMinion))
	{
		Minions.Remove(EnemyToRemove);

		if (DeactivatedMinion->OnDeactivated.IsBound())
		{
			DeactivatedMinion->OnDeactivated.RemoveDynamic(this, &APaladinBoss::HandleMinionDeactivated);
		}
	}
}

void APaladinBoss::HandleFloodDamage()
{
	bCanFloodDamage = true;
}
