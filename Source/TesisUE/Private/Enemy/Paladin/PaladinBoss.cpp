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
#include <Player/PlayerHeroController.h>


APaladinBoss::APaladinBoss()
{
	SpectralTrapComponent2 = CreateDefaultSubobject<USpectralTrapComponent>(TEXT("SpectralTrapComponent"));
	SpectralTrapComponent2->SphereCollider->SetupAttachment(GetRootComponent());

	AuraMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AuraMesh"));
	AuraMeshComponent->SetupAttachment(GetRootComponent());
}

void APaladinBoss::BeginPlay()
{
	Super::BeginPlay();

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SpawnPoint"), SpawnPoints);

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

				SpectralTrapComponent2->FinishDamaging();
				SpectralTrapComponent2->DestroyComponent();
				AuraMeshComponent->DestroyComponent();
			}
		);
	}
}

float APaladinBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (PlayerControllerRef)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Orange, FString("Valid Player Hero Controller..."));

		PlayerControllerRef->HandleBossHealth(Attributes->GetHealthPercent(), Attributes->GetShieldHealthPercent());
	}

	return DamageAmount;
}	

void APaladinBoss::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
{
	if (BBComponent->GetValueAsBool(FName("CanReceiveDamage")))
	{
		Super::GetHit_Implementation(DamageCauser, ImpactPoint, DamageType, DamageReceived);

		float DamageAccumulated = BBComponent->GetValueAsFloat(FName("DamageAccumulated"));
		BBComponent->SetValueAsFloat(FName("DamageAccumulated"), DamageAccumulated + DamageReceived);
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
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, FString::SanitizeFloat(Minions.Num()));
	return Minions.Num() <= 0;
}

void APaladinBoss::Invoke()
{
	UWorld* World = GetWorld();
	if (!World || !MinionToSpawnClass) return;

	UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>();
	if (!PoolManager) return;

	for (AActor* SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint)
		{
			FVector SpawnLocation = SpawnPoint->GetActorLocation();
			FRotator SpawnRotation = SpawnPoint->GetActorRotation();

			AEnemy* EnemyFromPool = PoolManager->SpawnEnemyFromPool(MinionToSpawnClass, SpawnLocation, SpawnRotation, this, this);

			if (EnemyFromPool)
			{
				EnemyFromPool->OnDeactivated.AddDynamic(this, &APaladinBoss::HandleMinionDeactivated);

				Minions.Add(EnemyFromPool);
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Failed to spawn minion from pool!"));
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
	Minions.Remove(DeactivatedMinion);

	if (DeactivatedMinion->OnDeactivated.IsBound())
	{
		DeactivatedMinion->OnDeactivated.RemoveDynamic(this, &APaladinBoss::HandleMinionDeactivated);
	}
}

void APaladinBoss::HandleFloodDamage()
{
	bCanFloodDamage = true;
}
