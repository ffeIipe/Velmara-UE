#include "Features/BossSystem/Components/BossSummonComponent.h"

#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
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
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickInterval(0.25f);
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
				const int32 Missing = PrewarmPerClass - Pool->GetInactiveCount(MinionClass);
				if (Missing > 0)
				{
					Pool->InitializePool(MinionClass, Missing);
				}
			}
		}

		for (const TSubclassOf<AActor>& RangedClass : RangedMinionClasses)
		{
			if (RangedClass)
			{
				const int32 Missing = PrewarmPerClass - Pool->GetInactiveCount(RangedClass);
				if (Missing > 0)
				{
					Pool->InitializePool(RangedClass, Missing);
				}
			}
		}
	}
}

void UBossSummonComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEncounterStarted || bBossDead || CurrentStage < FinalStageIndex || !HasValidMinionClass())
	{
		EmergencyStarvationStartTime = -1.0;
		return;
	}

	const UWorld* World = GetWorld();
	if (!World) return;

	PruneMinions();
	const int32 CurrentTotal = AliveMinions.Num() + PendingSpawns;

	if (CurrentTotal < MinGuaranteedMinionsFinalStage)
	{
		const double Now = World->GetTimeSeconds();
		if (EmergencyStarvationStartTime < 0.0)
		{
			EmergencyStarvationStartTime = Now;
		}

		if ((Now - EmergencyStarvationStartTime) >= AutoFallbackSummonDelay && Now >= NextEmergencySummonTime)
		{
			APawn* FallbackTarget = UGameplayStatics::GetPlayerPawn(this, 0);
			const int32 NeededCount = FMath::Max(MinGuaranteedMinionsFinalStage - CurrentTotal, 1);
			const int32 TotalBefore = GetAliveMinionCount() + PendingSpawns;
			SpawnMinionsNow(FallbackTarget, NeededCount);
			const int32 QueuedCount = (GetAliveMinionCount() + PendingSpawns) - TotalBefore;
			if (QueuedCount > 0)
			{
				NextEmergencySummonTime = Now + EmergencySummonCooldown;
				EmergencyStarvationStartTime = -1.0;
				OnEmergencySummonTriggered.Broadcast(QueuedCount);
			}
		}
	}
	else
	{
		EmergencyStarvationStartTime = -1.0;
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
	EmergencyStarvationStartTime = -1.0;
	NextEmergencySummonTime = 0.0;
	if (const UWorld* World = GetWorld())
	{
		NextSummonTime = World->GetTimeSeconds() + FirstSummonDelay;
	}
}

void UBossSummonComponent::SetCurrentStage(const int32 NewStage)
{
	CurrentStage = FMath::Max(0, NewStage);

	if (const UWorld* World = GetWorld(); World && bEncounterStarted)
	{
		const double Now = World->GetTimeSeconds();
		NextSummonTime = Now + SummonCooldown;

		if (CurrentStage >= FinalStageIndex)
		{
			NextEmergencySummonTime = Now + EmergencySummonCooldown;
			EmergencyStarvationStartTime = -1.0;
		}
		else
		{
			EmergencyStarvationStartTime = -1.0;
		}
	}
}

int32 UBossSummonComponent::GetEffectiveMaxAliveMinions() const
{
	return CurrentStage >= FinalStageIndex ? MaxAliveMinionsFinalStage : MaxAliveMinions;
}

bool UBossSummonComponent::HasValidMinionClass() const
{
	const auto IsValidClass = [](const TSubclassOf<AActor>& MinionClass) { return MinionClass != nullptr; };
	return MinionClasses.ContainsByPredicate(IsValidClass) || RangedMinionClasses.ContainsByPredicate(IsValidClass);
}

TSubclassOf<AActor> UBossSummonComponent::ChooseMinionClass() const
{
	TArray<TSubclassOf<AActor>> ValidMeleeClasses;
	for (const TSubclassOf<AActor>& MinionClass : MinionClasses)
	{
		if (MinionClass) ValidMeleeClasses.Add(MinionClass);
	}

	TArray<TSubclassOf<AActor>> ValidRangedClasses;
	for (const TSubclassOf<AActor>& RangedClass : RangedMinionClasses)
	{
		if (RangedClass) ValidRangedClasses.Add(RangedClass);
	}

	if (ValidRangedClasses.Num() > 0)
	{
		int32 AliveRangedCount = 0;
		for (const TWeakObjectPtr<AActor>& WeakMinion : AliveMinions)
		{
			const AActor* Minion = WeakMinion.Get();
			if (!IsMinionAlive(Minion)) continue;

			const UClass* MinionActualClass = Minion->GetClass();
			const bool bIsRanged = ValidRangedClasses.ContainsByPredicate([MinionActualClass](const TSubclassOf<AActor>& RangedClass)
			{
				return RangedClass && MinionActualClass->IsChildOf(RangedClass.Get());
			});
			if (bIsRanged)
			{
				++AliveRangedCount;
			}
		}

		const bool bNoMeleeAvailable = ValidMeleeClasses.Num() == 0;
		if (AliveRangedCount < MaxAliveRangedMinions && (bNoMeleeAvailable || FMath::FRand() < RangedMinionChance))
		{
			return ValidRangedClasses[FMath::RandRange(0, ValidRangedClasses.Num() - 1)];
		}

		if (bNoMeleeAvailable)
		{
			return ValidRangedClasses[FMath::RandRange(0, ValidRangedClasses.Num() - 1)];
		}
	}

	if (ValidMeleeClasses.Num() > 0)
	{
		return ValidMeleeClasses[FMath::RandRange(0, ValidMeleeClasses.Num() - 1)];
	}

	return nullptr;
}

bool UBossSummonComponent::NeedsEmergencySummon() const
{
	if (!bEncounterStarted || bBossDead || !HasValidMinionClass()) return false;
	if (CurrentStage < FinalStageIndex) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	if (World->GetTimeSeconds() < NextEmergencySummonTime) return false;

	return (GetAliveMinionCount() + PendingSpawns) < MinGuaranteedMinionsFinalStage;
}

bool UBossSummonComponent::StartEmergencySummon(AActor* Target)
{
	if (!NeedsEmergencySummon()) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	const int32 CurrentTotal = GetAliveMinionCount() + PendingSpawns;
	const int32 MaxAllowed = FMath::Max(1, GetEffectiveMaxAliveMinions() - CurrentTotal);
	const int32 NeededCount = FMath::Clamp(MinGuaranteedMinionsFinalStage - CurrentTotal, 1, MaxAllowed);

	const double Now = World->GetTimeSeconds();
	NextEmergencySummonTime = Now + EmergencySummonCooldown;
	EmergencyStarvationStartTime = -1.0;

	UBossCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UBossCombatComponent>() : nullptr;
	if (Combat && SummonMontage)
	{
		PendingSummonCountOverride = NeededCount;
		const TWeakObjectPtr<AActor> WeakTarget = Target;
		const bool bStarted = Combat->PlayActionMontage(SummonMontage, SummonPlayRate, SummonEventTime,
			FSimpleDelegate::CreateUObject(this, &UBossSummonComponent::HandleSummonEvent, WeakTarget, NeededCount),
			FBossActionFinished::CreateUObject(this, &UBossSummonComponent::HandleSummonActionFinished),
			Target, RecoveryAfterSummon);
		if (!bStarted)
		{
			PendingSummonCountOverride = 0;
			return false;
		}
		OnEmergencySummonTriggered.Broadcast(NeededCount);
		return true;
	}

	PendingSummonCountOverride = 0;
	const int32 Spawned = SpawnMinionsNow(Target, NeededCount);
	if (Spawned <= 0) return false;

	OnEmergencySummonTriggered.Broadcast(Spawned);
	return true;
}

bool UBossSummonComponent::CanSummon() const
{
	if (!bEncounterStarted || bBossDead || !HasValidMinionClass()) return false;
	if (CurrentStage < MinSummonStage) return false;
	if (NeedsEmergencySummon()) return true;

	const UWorld* World = GetWorld();
	if (!World) return false;

	return World->GetTimeSeconds() >= NextSummonTime && (GetAliveMinionCount() + PendingSpawns) < GetEffectiveMaxAliveMinions();
}

bool UBossSummonComponent::StartSummon(AActor* Target, const int32 CountOverride)
{
	if (!CanSummon()) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	const double Now = World->GetTimeSeconds();
	if (CountOverride <= 0 && Now < NextSummonTime && NeedsEmergencySummon())
	{
		return StartEmergencySummon(Target);
	}

	NextSummonTime = Now + SummonCooldown;
	PendingSummonCountOverride = 0;

	const int32 Count = CountOverride > 0 ? CountOverride : MinionsPerSummon;
	const TWeakObjectPtr<AActor> WeakTarget = Target;

	UBossCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UBossCombatComponent>() : nullptr;
	if (!Combat)
	{
		return SpawnMinionsNow(Target, Count) > 0;
	}

	return Combat->PlayActionMontage(SummonMontage, SummonPlayRate, SummonEventTime,
		FSimpleDelegate::CreateUObject(this, &UBossSummonComponent::HandleSummonEvent, WeakTarget, Count),
		FBossActionFinished::CreateUObject(this, &UBossSummonComponent::HandleSummonActionFinished),
		Target, RecoveryAfterSummon);
}

void UBossSummonComponent::HandleSummonEvent(const TWeakObjectPtr<AActor> Target, const int32 Count)
{
	const int32 EffectiveCount = PendingSummonCountOverride > 0 ? PendingSummonCountOverride : Count;
	PendingSummonCountOverride = 0;
	SpawnMinionsNow(Target.Get(), EffectiveCount);
}

void UBossSummonComponent::HandleSummonActionFinished(const bool bInterrupted)
{
	PendingSummonCountOverride = 0;
}

int32 UBossSummonComponent::SpawnMinionsNow(AActor* Target, const int32 Count)
{
	if (bBossDead || !HasValidMinionClass()) return 0;

	PruneMinions();

	const int32 Requested = Count > 0 ? Count : MinionsPerSummon;
	const int32 ToSpawn = FMath::Min(Requested, GetEffectiveMaxAliveMinions() - AliveMinions.Num() - PendingSpawns);
	if (ToSpawn <= 0) return 0;

	UWorld* World = GetWorld();
	if (!World || !GetOwner()) return 0;

	const FVector Center = GetOwner()->GetActorLocation();
	const TWeakObjectPtr<AActor> WeakTarget = Target;

	TArray<FVector> UsedLocations;
	int32 Scheduled = 0;

	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		FVector Location;
		if (!FindSpawnLocation(Center, UsedLocations, Location)) continue;

		UsedLocations.Add(Location);

		if (SpawnEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpawnEffect, Location);
		}

		const float Delay = SpawnDelay + SpawnStagger * Scheduled;
		++PendingSpawns;
		++Scheduled;

		if (Delay <= 0.f)
		{
			SpawnMinion(Location, WeakTarget);
			continue;
		}

		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &UBossSummonComponent::SpawnMinion, Location, WeakTarget), Delay, false);
		PendingSpawnTimers.Add(Handle);
	}

	if (AliveMinions.Num() + PendingSpawns >= MinGuaranteedMinionsFinalStage)
	{
		EmergencyStarvationStartTime = -1.0;
	}

	if (Scheduled > 0 && SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, Center);
	}

	return Scheduled;
}

void UBossSummonComponent::SpawnMinion(FVector Location, const TWeakObjectPtr<AActor> Target)
{
	PendingSpawns = FMath::Max(0, PendingSpawns - 1);
	if (const UWorld* World = GetWorld())
	{
		PendingSpawnTimers.RemoveAll([World](const FTimerHandle& Handle) { return !World->GetTimerManager().TimerExists(Handle); });
	}

	if (bBossDead) return;

	const TSubclassOf<AActor> MinionClass = ChooseMinionClass();
	if (!MinionClass) return;

	if (const ACharacter* CharacterDefaults = Cast<ACharacter>(MinionClass->GetDefaultObject()))
	{
		Location.Z += CharacterDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
	}

	FRotator Rotation(0.f, GetOwner() ? GetOwner()->GetActorRotation().Yaw : 0.f, 0.f);
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

	if (bForceMinionsDirectlyPossessable)
	{
		if (UTotemChargeComponent* ChargeComp = Minion->FindComponentByClass<UTotemChargeComponent>())
		{
			ChargeComp->SetRequiresCharge(false);
			ChargeComp->ActivatePossession(nullptr);
		}
	}

	AliveMinions.AddUnique(Minion);

	if (AEntity* Entity = Cast<AEntity>(Minion))
	{
		Entity->OnDead.RemoveDynamic(this, &UBossSummonComponent::HandleMinionDead);
		Entity->OnDead.AddDynamic(this, &UBossSummonComponent::HandleMinionDead);
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
	PendingSummonCountOverride = 0;
	EmergencyStarvationStartTime = -1.0;

	if (const UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : PendingSpawnTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	PendingSpawnTimers.Reset();
	PendingSpawns = 0;

	CleanupMinions(CleanupOnBossDeath);
}
