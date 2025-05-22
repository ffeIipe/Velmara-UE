#include "Enemy/Paladin/PaladinBoss.h"
#include <AI/EnemyAIController.h>
#include "BehaviorTree/BlackboardComponent.h"


APaladinBoss::APaladinBoss()
{
	USceneComponent* SP1 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint1"));
	if (SP1)
	{
		SP1->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP1);
	}
	USceneComponent* SP2 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint2"));
	if (SP2)
	{
		SP2->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP2);
	}
	USceneComponent* SP3 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint3"));
	if (SP3)
	{
		SP3->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP3);
	}
}

void APaladinBoss::BeginPlay()
{
	Super::BeginPlay();

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
	DamageCauserOf = DamageCauser;
	NotifyDamageTakenToBlackboard();
	return 0.0f;
}

void APaladinBoss::DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage)
{
	Super::DirectionalHitReact(ImpactPoint, HitReactAnimMontage);
}

void APaladinBoss::Attack()
{
	TryToInvoke();
}

void APaladinBoss::TryToInvoke()
{
	if (Minions.Num() <= 0)
	{
		Invoke();
	}
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

			if (APaladin* NewPaladin = Cast<APaladin>(EnemyFromPool))
			{
				NewPaladin->OnDeactivated.AddDynamic(this, &APaladinBoss::HandleMinionDeactivated);

				Minions.Add(NewPaladin);
			}
			else if (EnemyFromPool)
			{
				PoolManager->ReturnEnemyToPool(EnemyFromPool);
			}
		}
	}
}

void APaladinBoss::HandleMinionDeactivated(AEnemy* DeactivatedMinion)
{
	if (APaladin* PaladinToRemove = Cast<APaladin>(DeactivatedMinion))
	{
		Minions.Remove(PaladinToRemove);

		if (DeactivatedMinion->OnDeactivated.IsBound())
		{
			DeactivatedMinion->OnDeactivated.RemoveDynamic(this, &APaladinBoss::HandleMinionDeactivated);
		}
		
		TryToInvoke();
	}
}

void APaladinBoss::NotifyDamageTakenToBlackboard()
{
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->GetBlackboardComponent()->SetValueAsBool(FName("DamageTakenRecently"), true);
	}
}
