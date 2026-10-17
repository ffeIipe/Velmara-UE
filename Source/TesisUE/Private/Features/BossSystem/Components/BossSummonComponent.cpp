#include "Features/BossSystem/Components/BossSummonComponent.h"

#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "GameFramework/Character.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Interfaces/Damageable.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace BossSummon
{
	constexpr int32 MaxPlacementAttempts = 16;
	constexpr float MinSeparationBetweenMinions = 160.f;
}

UBossSummonComponent::UBossSummonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBossSummonComponent::BeginPlay()
{
	Super::BeginPlay();

	if (PrewarmPerClass <= 0) return;

	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		for (const TSubclassOf<AActor>& MinionClass : MinionClasses)
		{
			if (MinionClass)
			{
				Pool->InitializePool(MinionClass, PrewarmPerClass - Pool->GetInactiveCount(MinionClass));
			}
		}
	}
}

void UBossSummonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : PendingSpawnTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	PendingSpawnTimers.Reset();

	for (const TWeakObjectPtr<AActor>& Minion : AliveMinions)
	{
		if (AEntity* Entity = Cast<AEntity>(Minion.Get()))
		{
			Entity->OnDead.RemoveDynamic(this, &UBossSummonComponent::HandleMinionDead);
		}
	}
	AliveMinions.Reset();

	Super::EndPlay(EndPlayReason);
}

void UBossSummonComponent::NotifyEncounterStarted()
{
	if (bEncounterStarted) return;

	bEncounterStarted = true;
	NextSummonTime = GetWorld()->GetTimeSeconds() + FirstSummonDelay;
}

bool UBossSummonComponent::CanSummon() const
{
	if (!bEncounterStarted || bBossDead) return false;
	if (GetWorld()->GetTimeSeconds() < NextSummonTime) return false;
	if (!MinionClasses.ContainsByPredicate([](const TSubclassOf<AActor>& MinionClass) { return MinionClass != nullptr; })) return false;

	return GetAliveMinionCount() + PendingSpawns < MaxAliveMinions;
}

bool UBossSummonComponent::StartSummon(AActor* Target, const int32 CountOverride)
{
	if (!CanSummon()) return false;

	NextSummonTime = GetWorld()->GetTimeSeconds() + SummonCooldown;

	const int32 Count = CountOverride > 0 ? CountOverride : MinionsPerSummon;
	const TWeakObjectPtr<AActor> WeakTarget = Target;

	UBossCombatComponent* Combat = GetOwner()->FindComponentByClass<UBossCombatComponent>();
	if (!Combat)
	{
		return SpawnMinionsNow(Target, Count) > 0;
	}

	return Combat->PlayActionMontage(SummonMontage, SummonPlayRate, SummonEventTime,
		FSimpleDelegate::CreateUObject(this, &UBossSummonComponent::HandleSummonEvent, WeakTarget, Count),
		FBossActionFinished(), Target, RecoveryAfterSummon);
}

void UBossSummonComponent::HandleSummonEvent(const TWeakObjectPtr<AActor> Target, const int32 Count)
{
	SpawnMinionsNow(Target.Get(), Count);
}

int32 UBossSummonComponent::SpawnMinionsNow(AActor* Target, const int32 Count)
{
	if (bBossDead) return 0;

	PruneMinions();

	const int32 Requested = Count > 0 ? Count : MinionsPerSummon;
	const int32 ToSpawn = FMath::Min(Requested, MaxAliveMinions - AliveMinions.Num() - PendingSpawns);
	if (ToSpawn <= 0) return 0;

	TArray<TSubclassOf<AActor>> ValidClasses;
	for (const TSubclassOf<AActor>& MinionClass : MinionClasses)
	{
		if (MinionClass) ValidClasses.Add(MinionClass);
	}
	if (ValidClasses.Num() == 0) return 0;

	UWorld* World = GetWorld();
	const FVector Center = GetOwner()->GetActorLocation();
	const TWeakObjectPtr<AActor> WeakTarget = Target;

	TArray<FVector> UsedLocations;
	int32 Scheduled = 0;

	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		FVector Location;
		if (!FindSpawnLocation(Center, UsedLocations, Location)) continue;

		UsedLocations.Add(Location);

		const TSubclassOf<AActor> MinionClass = ValidClasses[FMath::RandRange(0, ValidClasses.Num() - 1)];

		// El efecto en el piso avisa dónde va a aparecer cada enemigo.
		if (SpawnEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpawnEffect, Location);
		}

		const float Delay = SpawnDelay + SpawnStagger * Scheduled;
		++PendingSpawns;
		++Scheduled;

		if (Delay <= 0.f)
		{
			SpawnMinion(MinionClass, Location, WeakTarget);
			continue;
		}

		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &UBossSummonComponent::SpawnMinion, MinionClass, Location, WeakTarget), Delay, false);
		PendingSpawnTimers.Add(Handle);
	}

	if (Scheduled > 0 && SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, Center);
	}

	return Scheduled;
}

void UBossSummonComponent::SpawnMinion(const TSubclassOf<AActor> MinionClass, FVector Location, const TWeakObjectPtr<AActor> Target)
{
	PendingSpawns = FMath::Max(0, PendingSpawns - 1);
	PendingSpawnTimers.RemoveAll([this](const FTimerHandle& Handle) { return !GetWorld()->GetTimerManager().TimerExists(Handle); });

	if (bBossDead || !MinionClass) return;

	// El pivote de un Character es el centro de la cápsula.
	if (const ACharacter* CharacterDefaults = Cast<ACharacter>(MinionClass->GetDefaultObject()))
	{
		Location.Z += CharacterDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
	}

	FRotator Rotation(0.f, GetOwner()->GetActorRotation().Yaw, 0.f);
	if (const AActor* TargetActor = Target.Get())
	{
		Rotation.Yaw = (TargetActor->GetActorLocation() - Location).Rotation().Yaw;
	}

	AActor* Minion = nullptr;
	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		Minion = Pool->GetActorFromPool(MinionClass, FTransform(Rotation, Location));
	}
	if (!Minion) return;

	AliveMinions.AddUnique(Minion);

	if (AEntity* Entity = Cast<AEntity>(Minion))
	{
		Entity->OnDead.AddUniqueDynamic(this, &UBossSummonComponent::HandleMinionDead);
	}

	OnMinionSpawned.Broadcast(Minion);
}

void UBossSummonComponent::HandleMinionDead(AEntity* DeadEntity)
{
	if (DeadEntity)
	{
		DeadEntity->OnDead.RemoveDynamic(this, &UBossSummonComponent::HandleMinionDead);
	}

	AliveMinions.RemoveAll([DeadEntity](const TWeakObjectPtr<AActor>& Minion) { return !Minion.IsValid() || Minion.Get() == DeadEntity; });
}

bool UBossSummonComponent::FindSpawnLocation(const FVector& Center, const TArray<FVector>& UsedLocations, FVector& OutLocation) const
{
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	for (int32 Attempt = 0; Attempt < BossSummon::MaxPlacementAttempts; ++Attempt)
	{
		FVector Candidate;

		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (!NavSystem->GetRandomReachablePointInRadius(Center, MaxSpawnRadius, NavLocation)) continue;

			Candidate = NavLocation.Location;
			if (FVector::Dist2D(Candidate, Center) < MinSpawnRadius) continue;
		}
		else
		{
			const float Distance = FMath::FRandRange(MinSpawnRadius, MaxSpawnRadius);
			Candidate = Center + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * Distance;
		}

		const bool bTooClose = UsedLocations.ContainsByPredicate([&Candidate](const FVector& Used)
		{
			return FVector::Dist2D(Used, Candidate) < BossSummon::MinSeparationBetweenMinions;
		});
		if (bTooClose) continue;

		OutLocation = Candidate;
		return true;
	}

	return false;
}

bool UBossSummonComponent::IsMinionAlive(const AActor* Minion) const
{
	if (!IsValid(Minion) || Minion->IsHidden()) return false;

	if (const AEntity* Entity = Cast<AEntity>(Minion); Entity && !Entity->IsAlive()) return false;

	const UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	return !Pool || !Pool->IsActorInPool(Minion);
}

int32 UBossSummonComponent::GetAliveMinionCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Minion : AliveMinions)
	{
		if (IsMinionAlive(Minion.Get())) ++Count;
	}
	return Count;
}

TArray<AActor*> UBossSummonComponent::GetAliveMinions() const
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& Minion : AliveMinions)
	{
		if (AActor* MinionActor = Minion.Get(); IsMinionAlive(MinionActor))
		{
			Result.Add(MinionActor);
		}
	}
	return Result;
}

void UBossSummonComponent::PruneMinions()
{
	for (int32 Index = AliveMinions.Num() - 1; Index >= 0; --Index)
	{
		AActor* Minion = AliveMinions[Index].Get();
		if (IsMinionAlive(Minion)) continue;

		if (AEntity* Entity = Cast<AEntity>(Minion))
		{
			Entity->OnDead.RemoveDynamic(this, &UBossSummonComponent::HandleMinionDead);
		}
		AliveMinions.RemoveAt(Index);
	}
}

void UBossSummonComponent::CleanupMinions(const EBossMinionCleanup Mode)
{
	if (Mode == EBossMinionCleanup::Keep) return;

	PruneMinions();

	UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	const TArray<TWeakObjectPtr<AActor>> Minions = AliveMinions;

	for (const TWeakObjectPtr<AActor>& WeakMinion : Minions)
	{
		AActor* Minion = WeakMinion.Get();
		if (!Minion) continue;

		if (Mode == EBossMinionCleanup::Kill && Minion->Implements<UDamageable>())
		{
			FGameplayEventData DeathPayload;
			DeathPayload.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Death"), false);
			DeathPayload.Instigator = GetOwner();
			DeathPayload.Target = Minion;
			DeathPayload.InstigatorTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Death.Default"), false));

			IDamageable::Execute_MortalDamage(Minion, DeathPayload);
		}
		else if (Pool)
		{
			Pool->ReturnToPool(Minion);
		}
	}
}

void UBossSummonComponent::OnBossDied()
{
	bBossDead = true;

	for (FTimerHandle& Handle : PendingSpawnTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(Handle);
	}
	PendingSpawnTimers.Reset();
	PendingSpawns = 0;

	CleanupMinions(CleanupOnBossDeath);
}
