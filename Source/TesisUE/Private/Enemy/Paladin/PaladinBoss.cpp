#include "Enemy/Paladin/PaladinBoss.h"
#include <AI/EnemyAIController.h>
#include "BehaviorTree/BlackboardComponent.h"

#include "Components/AttributeComponent.h"
#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include <Kismet/KismetMathLibrary.h>


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
	if (Attributes->IsAlive())
	{
		if (Attributes->IsShielded() && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
		{
			DamageCauserOf = DamageCauser;
			NotifyDamageTakenToBlackboard();
			Attributes->ReceiveShieldDamage(DamageAmount);
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Purple, FString("RECEIVING SHIELD DAMAGE"));
		}
		else
		{
			DamageCauserOf = DamageCauser;
			NotifyDamageTakenToBlackboard();
			Attributes->ReceiveDamage(DamageAmount);
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, FString("RECEIVING DAMAGE"));
		}
	}
	else
	{
		Die(DamageCauser);
	}

	return DamageAmount;
}

void APaladinBoss::DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage)
{
	if (UKismetMathLibrary::RandomBool())
	{
		PlayAnimMontage(HitReactAnimMontage, 1.f, FName("FromRight"));
	}
	else
	{
		PlayAnimMontage(HitReactAnimMontage, 1.f, FName("FromLeft"));
	}
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
