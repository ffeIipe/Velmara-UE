#include "Enemy/Paladin/PaladinBoss.h"
#include <AI/EnemyAIController.h>
#include "BehaviorTree/BlackboardComponent.h"

#include "Components/AttributeComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "SpectralTrapComponent.h"

#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>
#include <Player/PlayerHeroController.h>

#include "Subsystems/EnemyPoolManager.h"


class UEnemyPoolManager;

APaladinBoss::APaladinBoss()
{
	SpectralTrapComponent2 = CreateDefaultSubobject<USpectralTrapComponent>(TEXT("SpectralTrapComponent"));
	SpectralTrapComponent2->SphereCollider->SetupAttachment(GetRootComponent());

	AuraMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AuraMesh"));
	AuraMeshComponent->SetupAttachment(GetRootComponent());
	bShouldDropOrbs = false;
}

void APaladinBoss::BeginPlay()
{
	Super::BeginPlay();

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SpawnPoint"), SpawnPoints);

	GetAttributeComponent()->OnDettachShield.AddDynamic(this, &APaladinBoss::ShieldDetach);
	
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

void APaladinBoss::ShieldDetach()
{
	SpectralTrapComponent2->FinishDamaging();
	SpectralTrapComponent2->DestroyComponent();
	AuraMeshComponent->DestroyComponent();
}

float APaladinBoss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (PlayerControllerRef)
	{
		Cast<APlayerHeroController>(PlayerControllerRef)->HandleBossHealth(GetAttributeComponent()->GetHealthPercent(), GetAttributeComponent()->GetShieldHealthPercent());
	}

	return DamageAmount;
}

void APaladinBoss::GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	FDamageEvent const& DamageEvent, const float DamageReceived)
{
	Super::GetHit(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

	if (BBComponent)
	{
		if (BBComponent->GetValueAsBool(FName("CanReceiveDamage")))
		{
			Super::GetHit(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

			const float DamageAccumulated = BBComponent->GetValueAsFloat(FName("DamageAccumulated"));
			BBComponent->SetValueAsFloat(FName("DamageAccumulated"), DamageAccumulated + DamageReceived);
		}
	}
	else
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

		if (BBComponent->GetValueAsBool(FName("CanReceiveDamage")))
		{
			Super::GetHit(DamageCauser, ImpactPoint, DamageEvent, DamageReceived);

			const float DamageAccumulated = BBComponent->GetValueAsFloat(FName("DamageAccumulated"));
			BBComponent->SetValueAsFloat(FName("DamageAccumulated"), DamageAccumulated + DamageReceived);
		}
	}
}

void APaladinBoss::Die(UAnimMontage* DeathAnim, FName Section)
{
	Super::Die(DeathAnim, Section);
	
	if (OnDead.IsBound())
	{
		OnDead.Broadcast(this);
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
	const UWorld* World = GetWorld();
	if (!World || !MinionToSpawnClass) return;

	UEnemyPoolManager* PoolManager = World->GetSubsystem<UEnemyPoolManager>();
	if (!PoolManager) return;

	for (const AActor* SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint)
		{
			FVector SpawnLocation = SpawnPoint->GetActorLocation();
			FRotator SpawnRotation = SpawnPoint->GetActorRotation();

			if (AEnemy* EnemyFromPool = PoolManager->SpawnEnemyFromPool(MinionToSpawnClass, SpawnLocation, SpawnRotation, this, this))
			{
				EnemyFromPool->OnDeactivated.AddDynamic(this, &APaladinBoss::HandleMinionDeactivated);

				Minions.Add(EnemyFromPool);
			}
		}
	}
}

void APaladinBoss::FloodAttack()
{
	if (FloodToRaise)
	{
		Execute_StopAnimMontage(this, GetCurrentMontage());
		Execute_PlayAnimMontage(this, InvokeMontage, 1.f, "Default");

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
