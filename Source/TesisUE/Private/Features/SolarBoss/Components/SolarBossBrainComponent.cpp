#include "Features/SolarBoss/Components/SolarBossBrainComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"
#include "Features/BossSystem/Actors/BossLightShaft.h"
#include "Features/BossSystem/Core/BossSystemLibrary.h"
#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GAS/VelmaraGameplayTags.h"
#include "InteractionSystem/Interfaces/SignalReceiver.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "HUD/PaladinBossHealthBar.h"
#include "Interfaces/Damageable.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CustomPlayerController.h"

namespace SolarBossBrain
{
	constexpr int32 MaxPlacementAttempts = 16;
	constexpr float MinSeparationBetweenMinions = 160.f;
	constexpr float MinShaftHeight = 150.f;
}

USolarBossBrainComponent::USolarBossBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool USolarBossBrainComponent::IsActorAlive(const AActor* Actor)
{
	if (!IsValid(Actor)) return false;

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
	if (!ASC || !ASC->HasAttributeSetForAttribute(UVelmaraAttributeSet::GetHealthAttribute())) return true;

	return ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) > 0.f;
}

void USolarBossBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerEntity = Cast<AEntity>(GetOwner());
	if (!Config || !OwnerEntity || !OwnerEntity->GetAbilitySystemComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("SolarBossBrain on %s needs a Config and an AEntity owner"), *GetNameSafe(GetOwner()));
		return;
	}

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &USolarBossBrainComponent::InitializeBrain);
}

void USolarBossBrainComponent::InitializeBrain()
{
	UAbilitySystemComponent* ASC = OwnerEntity->GetAbilitySystemComponent();
	if (!OwnerEntity->HasActorBegunPlay() || !ASC->GetAvatarActor())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &USolarBossBrainComponent::InitializeBrain);
		return;
	}

	if (Config->MaxHealthOverride > 0.f)
	{
		ASC->SetNumericAttributeBase(UVelmaraAttributeSet::GetMaxHealthAttribute(), Config->MaxHealthOverride);
		ASC->SetNumericAttributeBase(UVelmaraAttributeSet::GetHealthAttribute(), Config->MaxHealthOverride);
	}

	ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &USolarBossBrainComponent::HandleEffectApplied);
	ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetHealthAttribute())
		.AddUObject(this, &USolarBossBrainComponent::HandleHealthChanged);

	OwnerEntity->bKnockbackResistant = false;
	ArenaOrigin = OwnerEntity->GetActorLocation();

	for (TActorIterator<ABossCeilingBreakPoint> It(GetWorld()); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), ArenaOrigin) <= Config->ArenaRadius)
		{
			BreakPoints.Add(*It);
		}
	}

	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		TArray<TSubclassOf<AActor>> MinionClasses = Config->DefaultMinionClasses;
		for (const FSolarBossCycleConfig& Cycle : Config->Cycles)
		{
			for (const TSubclassOf<AActor>& MinionClass : Cycle.MinionClassesOverride)
			{
				MinionClasses.AddUnique(MinionClass);
			}
		}

		for (const TSubclassOf<AActor>& MinionClass : MinionClasses)
		{
			if (MinionClass)
			{
				Pool->InitializePool(MinionClass, Config->PrewarmMinionsPerClass - Pool->GetInactiveCount(MinionClass));
			}
		}

		if (UClass* ShaftClass = Config->LightShaftClass.Get())
		{
			Pool->InitializePool(ShaftClass, Config->PrewarmLightShafts - Pool->GetInactiveCount(ShaftClass));
		}
	}

	if (ACustomPlayerController* PlayerController = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->PostPossessionSunGraceDuration = Config->PostPossessionSunGraceDuration;
	}

	AttackReadyTimes.Init(0.0, Config->MeleeAttacks.Num());
	bInitialized = true;
	TickAfterWalkSpeedWritersAndBeforeMovement();
	ApplyWalkSpeed();
	RefreshSuperArmor();
}

void USolarBossBrainComponent::TickAfterWalkSpeedWritersAndBeforeMovement()
{
	UCharacterMovementComponent* Movement = OwnerEntity->GetCharacterMovement();

	for (UActorComponent* Sibling : TInlineComponentArray<UActorComponent*>(OwnerEntity))
	{
		if (Sibling != this && Sibling != Movement && !Sibling->IsA<USceneComponent>())
		{
			AddTickPrerequisiteComponent(Sibling);
		}
	}

	Movement->AddTickPrerequisiteComponent(this);
}

void USolarBossBrainComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bInitialized)
	{
		ApplyWalkSpeed();
	}
}

void USolarBossBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bInitialized = false;

	if (UAbilitySystemComponent* ASC = OwnerEntity ? OwnerEntity->GetAbilitySystemComponent() : nullptr)
	{
		ASC->OnGameplayEffectAppliedDelegateToSelf.RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetHealthAttribute()).RemoveAll(this);
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	HideHealthBar();

	Super::EndPlay(EndPlayReason);
}

void USolarBossBrainComponent::StartEncounter()
{
	if (!IsReady() || bEncounterStarted) return;

	bEncounterStarted = true;

	if (Config->HealthBarClass && !HealthBarWidget)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			HealthBarWidget = CreateWidget<UPaladinBossHealthBar>(PlayerController, Config->HealthBarClass);
			if (HealthBarWidget)
			{
				HealthBarWidget->AddToViewport();
			}
		}
	}
	UpdateHealthBar();

	if (Mode == ESolarBossMode::Inactive)
	{
		SetMode(ESolarBossMode::BullyMelee);
	}

	SignalEncounterListeners(true);
}

void USolarBossBrainComponent::SignalEncounterListeners(const bool bActive) const
{
	for (AActor* Listener : EncounterListeners)
	{
		if (IsValid(Listener) && Listener->Implements<USignalReceiver>())
		{
			ISignalReceiver::Execute_ReceiveSignal(Listener, bActive, GetOwner());
		}
	}
}

void USolarBossBrainComponent::UpdateHealthBar() const
{
	if (!HealthBarWidget || !OwnerEntity) return;

	const UAbilitySystemComponent* ASC = OwnerEntity->GetAbilitySystemComponent();
	const float MaxHealth = ASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute());
	const float Percent = MaxHealth > 0.f
		? FMath::Clamp(ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) / MaxHealth, 0.f, 1.f)
		: 0.f;

	HealthBarWidget->ReceivePercentagesFromBoss(Percent, 0.f);
}

void USolarBossBrainComponent::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->RemoveFromParent();
		HealthBarWidget = nullptr;
	}
}

void USolarBossBrainComponent::HandleEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	if (bApplyingBonusDamage) return;

	AActor* Attacker = Source ? Source->GetAvatarActor() : nullptr;
	if (!Attacker)
	{
		Attacker = Spec.GetContext().GetEffectCauser();
	}

	if (Attacker && Attacker != GetOwner())
	{
		PendingHitAttacker = Attacker;
	}
}

void USolarBossBrainComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!IsReady()) return;

	UpdateHealthBar();

	if (Data.OldValue > 0.f && Data.NewValue <= 0.f)
	{
		HandleDeath();
		return;
	}

	if (Data.NewValue >= Data.OldValue) return;

	if (!bApplyingBonusDamage)
	{
		PendingHitLoss += Data.OldValue - Data.NewValue;
		if (!bHitScheduled)
		{
			bHitScheduled = true;
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &USolarBossBrainComponent::ProcessPendingHit);
		}
	}

	const float MaxHealth = OwnerEntity->GetAbilitySystemComponent()->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f) return;

	const float HealthPercent = Data.NewValue / MaxHealth;
	bool bTriggered = false;

	while (Config->Cycles.IsValidIndex(NextCycleIndex) && HealthPercent <= Config->Cycles[NextCycleIndex].HealthThreshold)
	{
		const int32 CycleIndex = NextCycleIndex++;
		PendingCycles.Add(CycleIndex);
		OnCycleTriggered.Broadcast(CycleIndex);
		bTriggered = true;
	}

	if (!bTriggered) return;

	StartEncounter();

	if (Config->bTriggerCycleEscapeImmediately)
	{
		bCycleBreakArmed = true;
		CancelHitReact();
	}

	SetMode(ESolarBossMode::CycleBreakEscape);
}

void USolarBossBrainComponent::ProcessPendingHit()
{
	const float Loss = PendingHitLoss;
	AActor* Attacker = PendingHitAttacker.Get();
	PendingHitLoss = 0.f;
	PendingHitAttacker.Reset();
	bHitScheduled = false;

	if (!IsReady()) return;

	StartEncounter();

	const ACustomPlayerController* PlayerController = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const float Multiplier = Config->PossessedMinionDamageMultiplier;
	if (PlayerController && Attacker && Multiplier > 1.f && PlayerController->IsPossessedHost(Attacker))
	{
		const float Bonus = Loss * (Multiplier - 1.f);

		bApplyingBonusDamage = true;
		UBossSystemLibrary::ApplyBossDamage(GetOwner(), Attacker, GetOwner(), MakeDamageParams(Bonus, FGameplayTag()), 1.f, FHitResult());
		bApplyingBonusDamage = false;

		OnHitByPossessedMinion.Broadcast(Attacker, Loss + Bonus);
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Mode != ESolarBossMode::BullyMelee || Now < NextDefensiveDodgeTime) return;

	RecentHitTimes.Add(Now);
	RecentHitTimes.RemoveAll([this, Now](const double Time) { return Now - Time > Config->ConsecutiveHitWindow; });

	if (RecentHitTimes.Num() >= Config->ConsecutiveHitsForDefensiveDodge)
	{
		bWantsDefensiveDodge = true;
		RecentHitTimes.Reset();
	}
}

void USolarBossBrainComponent::SetMode(const ESolarBossMode NewMode)
{
	const ESolarBossMode OldMode = Mode;
	Mode = NewMode;

	if (Mode != ESolarBossMode::BullyMelee)
	{
		bWantsDefensiveDodge = false;
		RecentHitTimes.Reset();
	}

	ApplyWalkSpeed();
	RefreshSuperArmor();

	if (OldMode != NewMode)
	{
		OnModeChanged.Broadcast(NewMode, OldMode);
	}
}

void USolarBossBrainComponent::ApplyWalkSpeed()
{
	if (!OwnerEntity || bDead) return;

	const float Base = Mode == ESolarBossMode::EvadeWithMinions ? Config->EvadeWalkSpeed : Config->BullyWalkSpeed;
	const float Bonus = Config->Cycles.IsValidIndex(CompletedCycles - 1) ? Config->Cycles[CompletedCycles - 1].WalkSpeedBonus : 0.f;

	OwnerEntity->GetCharacterMovement()->MaxWalkSpeed = Base + Bonus;
}

void USolarBossBrainComponent::EvaluateState(AActor* Target)
{
	if (!IsReady() || !bEncounterStarted) return;

	PruneMinions();

	if (PendingCycles.Num() > 0)
	{
		bCycleBreakArmed = bCycleBreakArmed || !IsInHitReact();
		bNeedsEmergencySummon = false;
		return;
	}

	UpdateModeFromMinions();

	if (!bFullCollapseActive || Minions.Num() + PendingSpawns >= Config->MinGuaranteedMinionsFinalCycle)
	{
		bNeedsEmergencySummon = false;
		StarvationStartTime = -1.0;
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (StarvationStartTime < 0.0)
	{
		StarvationStartTime = Now;
	}

	bNeedsEmergencySummon = Now >= NextEmergencySummonTime;
	if (bNeedsEmergencySummon && Now - StarvationStartTime > Config->EmergencyFallbackDelay)
	{
		SummonEmergencyMinions(Target);
	}
}

void USolarBossBrainComponent::UpdateModeFromMinions()
{
	const bool bEvade = Config->Cycles.IsValidIndex(CompletedCycles - 1)
		&& (bFullCollapseActive || GetAliveMinionCount() + PendingSpawns >= Config->Cycles[CompletedCycles - 1].MinMinionsToStayEvading);

	const ESolarBossMode Desired = bEvade ? ESolarBossMode::EvadeWithMinions : ESolarBossMode::BullyMelee;
	if (Mode != Desired)
	{
		SetMode(Desired);
	}
}

bool USolarBossBrainComponent::IsReactiveDodgeReady() const
{
	return GetWorld()->GetTimeSeconds() >= NextReactiveDodgeTime;
}

void USolarBossBrainComponent::SetTemporarySuperArmor(const bool bEnabled)
{
	bManualSuperArmor = bEnabled;
	RefreshSuperArmor();
}

void USolarBossBrainComponent::GrantSuperArmor(const float Duration)
{
	if (!IsReady() || Duration <= 0.f) return;

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!bTimedSuperArmor || TimerManager.GetTimerRemaining(SuperArmorTimer) < Duration)
	{
		bTimedSuperArmor = true;
		TimerManager.SetTimer(SuperArmorTimer, this, &USolarBossBrainComponent::ExpireSuperArmor, Duration, false);
	}

	RefreshSuperArmor();
}

void USolarBossBrainComponent::ExpireSuperArmor()
{
	bTimedSuperArmor = false;
	RefreshSuperArmor();
}

void USolarBossBrainComponent::RefreshSuperArmor()
{
	UAbilitySystemComponent* ASC = bInitialized && OwnerEntity ? OwnerEntity->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return;

	const bool bArmor = !bDead && (bManualSuperArmor || bTimedSuperArmor
		|| (PendingCycles.Num() > 0 && Config->bTriggerCycleEscapeImmediately));
	const bool bHitReactDisabled = !bDead && !Config->bAllowHitReactInBullyMode
		&& (Mode == ESolarBossMode::Inactive || Mode == ESolarBossMode::BullyMelee);
	const bool bBlockHitReact = bArmor || bHitReactDisabled;

	OwnerEntity->bKnockbackResistant = bBlockHitReact;

	if (bArmor != bAddedDodgeTag)
	{
		bAddedDodgeTag = bArmor;
		if (bArmor)
		{
			ASC->AddLooseGameplayTag(FVelmaraGameplayTags::Get().State_Dodging);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(FVelmaraGameplayTags::Get().State_Dodging);
		}
	}

	if (bBlockHitReact && BlockedHitReactTags.IsEmpty() && !Config->HitReactAbilityTags.IsEmpty())
	{
		BlockedHitReactTags = Config->HitReactAbilityTags;
		ASC->BlockAbilitiesWithTags(BlockedHitReactTags);
	}
	else if (!bBlockHitReact && !BlockedHitReactTags.IsEmpty())
	{
		ASC->UnBlockAbilitiesWithTags(BlockedHitReactTags);
		BlockedHitReactTags.Reset();
	}
}

void USolarBossBrainComponent::CancelHitReact()
{
	if (!IsReady()) return;

	OwnerEntity->GetAbilitySystemComponent()->CancelAbilities(&Config->HitReactAbilityTags);
	OwnerEntity->StopAnimMontage();
}

bool USolarBossBrainComponent::IsInHitReact(const UAnimMontage* IgnoredMontage) const
{
	if (!IsReady()) return false;

	const UAbilitySystemComponent* ASC = OwnerEntity->GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Stunned)) return true;

	if (!Config->HitReactAbilityTags.IsEmpty())
	{
		return ASC->GetActivatableAbilities().ContainsByPredicate([this](const FGameplayAbilitySpec& Spec)
		{
			return Spec.IsActive() && Spec.Ability && Spec.Ability->GetAssetTags().HasAny(Config->HitReactAbilityTags);
		});
	}

	const UAnimInstance* AnimInstance = OwnerEntity->GetMesh()->GetAnimInstance();
	const UAnimMontage* Active = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;

	return Active && Active != IgnoredMontage && !ASC->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Attacking);
}

void USolarBossBrainComponent::NotifyDodge(const ESolarBossDodgeMode DodgeMode)
{
	if (!IsReady()) return;

	CancelHitReact();
	GrantSuperArmor(Config->DodgeSuperArmorDuration);

	bWantsDefensiveDodge = false;
	RecentHitTimes.Reset();

	const double Now = GetWorld()->GetTimeSeconds();
	if (DodgeMode == ESolarBossDodgeMode::DefensiveAntiStunlock)
	{
		NextDefensiveDodgeTime = Now + Config->DefensiveDodgeCooldown;
	}
	else if (DodgeMode == ESolarBossDodgeMode::ReactiveEvade)
	{
		NextReactiveDodgeTime = Now + Config->EvadeReactiveDodgeCooldown;
	}
}

bool USolarBossBrainComponent::CanActInWorld() const
{
	return IsReady() && !GetOwner()->IsActorBeingDestroyed() && !GetWorld()->bIsTearingDown;
}

bool USolarBossBrainComponent::BeginCycleBreak()
{
	if (!CanActInWorld() || PendingCycles.Num() == 0) return false;

	if (ActiveCycleIndex != PendingCycles[0])
	{
		ActiveCycleIndex = PendingCycles[0];
		bCeilingBroken = false;
		bCycleMinionsSummoned = false;
	}
	bCycleBreakArmed = true;

	return true;
}

void USolarBossBrainComponent::ExecuteCeilingBreak(AActor* Target)
{
	if (!CanActInWorld() || bCeilingBroken || !Config->Cycles.IsValidIndex(ActiveCycleIndex)) return;

	bCeilingBroken = true;
	const FVector BossLocation = GetOwner()->GetActorLocation();

	if (Config->BreakCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this, Config->BreakCameraShake, BossLocation, 0.f, Config->BreakCameraShakeRadius);
	}

	if (Config->BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Config->BreakSound, BossLocation);
	}

	if (Config->BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Config->BreakEffect, BossLocation);
	}

	ApplyShockwave();

	if (Config->Cycles[ActiveCycleIndex].bFullCeilingCollapse)
	{
		CollapseWholeCeiling();
	}
	else
	{
		OpenPartialShafts(ActiveCycleIndex, Target);
	}
}

void USolarBossBrainComponent::SummonCycleMinions(AActor* Target)
{
	if (!CanActInWorld() || bCycleMinionsSummoned || !Config->Cycles.IsValidIndex(ActiveCycleIndex)) return;

	bCycleMinionsSummoned = true;
	SummonMinions(Target, Config->Cycles[ActiveCycleIndex].MinionCountToSpawn);
}

void USolarBossBrainComponent::CompleteCycleBreak()
{
	if (!CanActInWorld() || !Config->Cycles.IsValidIndex(ActiveCycleIndex)) return;

	bFullCollapseActive = bFullCollapseActive || Config->Cycles[ActiveCycleIndex].bFullCeilingCollapse;
	CompletedCycles = ActiveCycleIndex + 1;
	PendingCycles.Remove(ActiveCycleIndex);
	ActiveCycleIndex = INDEX_NONE;
	bCycleBreakArmed = PendingCycles.Num() > 0 && Config->bTriggerCycleEscapeImmediately;
	NextEmergencySummonTime = GetWorld()->GetTimeSeconds() + Config->EmergencySummonCooldown;
	StarvationStartTime = -1.0;

	if (PendingCycles.Num() == 0)
	{
		UpdateModeFromMinions();
	}

	RefreshSuperArmor();
}

void USolarBossBrainComponent::SummonEmergencyMinions(AActor* Target)
{
	if (!CanActInWorld() || !bFullCollapseActive) return;

	PruneMinions();

	const int32 Missing = Config->MinGuaranteedMinionsFinalCycle - Minions.Num() - PendingSpawns;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Missing <= 0 || Now < NextEmergencySummonTime) return;

	NextEmergencySummonTime = Now + Config->EmergencySummonCooldown;
	StarvationStartTime = -1.0;
	bNeedsEmergencySummon = false;
	SummonMinions(Target, Missing);
}

const FSolarBossCycleConfig* USolarBossBrainComponent::GetCurrentCycle() const
{
	const int32 Index = ActiveCycleIndex != INDEX_NONE ? ActiveCycleIndex : CompletedCycles - 1;
	return Config->Cycles.IsValidIndex(Index) ? &Config->Cycles[Index] : nullptr;
}

void USolarBossBrainComponent::SummonMinions(AActor* Target, const int32 Count)
{
	PruneMinions();

	const FSolarBossCycleConfig* Cycle = GetCurrentCycle();
	const bool bFinal = bFullCollapseActive || (Cycle && Cycle->bFullCeilingCollapse);
	int32 Capacity = bFinal ? Config->MaxAliveMinionsFinalCycle : Config->MaxAliveMinions;
	if (Cycle && Cycle->MaxAliveMinionsOverride > 0)
	{
		Capacity = Cycle->MaxAliveMinionsOverride;
	}

	TArray<TSubclassOf<AActor>> Classes = Cycle && Cycle->MinionClassesOverride.Num() > 0
		? Cycle->MinionClassesOverride
		: Config->DefaultMinionClasses;
	Classes.RemoveAll([](const TSubclassOf<AActor>& MinionClass) { return !MinionClass; });

	const int32 ToSpawn = FMath::Min(Count, Capacity - Minions.Num() - PendingSpawns);
	if (ToSpawn <= 0 || Classes.Num() == 0) return;

	const TWeakObjectPtr<AActor> WeakTarget = Target;
	TArray<FVector> UsedLocations;
	int32 Scheduled = 0;

	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		FVector Location;
		if (!FindSpawnLocation(UsedLocations, Location)) continue;

		UsedLocations.Add(Location);

		if (Config->SpawnEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Config->SpawnEffect, Location);
		}

		const TSubclassOf<AActor> MinionClass = Classes[FMath::RandRange(0, Classes.Num() - 1)];
		const float Delay = Config->SpawnDelay + Config->SpawnStagger * Scheduled;
		++PendingSpawns;
		++Scheduled;

		if (Delay <= 0.f)
		{
			SpawnMinion(MinionClass, Location, WeakTarget);
			continue;
		}

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &USolarBossBrainComponent::SpawnMinion, MinionClass, Location, WeakTarget), Delay, false);
	}

	if (Scheduled > 0 && Config->SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Config->SpawnSound, GetOwner()->GetActorLocation());
	}
}

void USolarBossBrainComponent::SpawnMinion(const TSubclassOf<AActor> MinionClass, FVector Location, const TWeakObjectPtr<AActor> Target)
{
	PendingSpawns = FMath::Max(0, PendingSpawns - 1);
	if (!CanActInWorld() || !MinionClass) return;

	if (const ACharacter* CharacterDefaults = Cast<ACharacter>(MinionClass->GetDefaultObject()))
	{
		Location.Z += CharacterDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
	}

	FRotator Rotation(0.f, GetOwner()->GetActorRotation().Yaw, 0.f);
	if (const AActor* TargetActor = Target.Get())
	{
		Rotation.Yaw = (TargetActor->GetActorLocation() - Location).Rotation().Yaw;
	}

	UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	AActor* Minion = Pool ? Pool->GetActorFromPool(MinionClass, FTransform(Rotation, Location)) : nullptr;
	if (!Minion) return;

	if (!IsActorAlive(Minion))
	{
		if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion))
		{
			MinionASC->SetNumericAttributeBase(UVelmaraAttributeSet::GetHealthAttribute(),
				MinionASC->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute()));
		}
	}

	if (Config->bForceMinionsPossessable)
	{
		if (UTotemChargeComponent* ChargeComponent = Minion->FindComponentByClass<UTotemChargeComponent>())
		{
			ChargeComponent->SetRequiresCharge(false);
			ChargeComponent->ActivatePossession(nullptr);
			ChargeComponent->ActivatePossession(nullptr);
		}
	}

	Minions.AddUnique(Minion);
}

bool USolarBossBrainComponent::FindSpawnLocation(const TArray<FVector>& UsedLocations, FVector& OutLocation) const
{
	const FVector BossLocation = GetOwner()->GetActorLocation();

	return TryFindSpawnLocation(BossLocation, Config->MinSpawnRadius, UsedLocations, OutLocation)
		|| TryFindSpawnLocation(BossLocation, 0.f, UsedLocations, OutLocation)
		|| TryFindSpawnLocation(ArenaOrigin, 0.f, UsedLocations, OutLocation);
}

bool USolarBossBrainComponent::TryFindSpawnLocation(const FVector& Center, const float MinRadius, const TArray<FVector>& UsedLocations,
	FVector& OutLocation) const
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	for (int32 Attempt = 0; Attempt < SolarBossBrain::MaxPlacementAttempts; ++Attempt)
	{
		FVector Candidate;

		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (!NavSystem->GetRandomReachablePointInRadius(Center, Config->MaxSpawnRadius, NavLocation)) continue;

			Candidate = NavLocation.Location;
			if (FVector::Dist2D(Candidate, Center) < MinRadius) continue;
		}
		else
		{
			const float Distance = FMath::FRandRange(MinRadius, Config->MaxSpawnRadius);
			Candidate = Center + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * Distance;
		}

		const bool bTooClose = UsedLocations.ContainsByPredicate([&Candidate](const FVector& Used)
		{
			return FVector::Dist2D(Used, Candidate) < SolarBossBrain::MinSeparationBetweenMinions;
		});
		if (bTooClose) continue;

		OutLocation = Candidate;
		return true;
	}

	return false;
}

bool USolarBossBrainComponent::IsMinionAlive(const AActor* Minion) const
{
	if (!IsActorAlive(Minion) || Minion->IsHidden()) return false;

	const UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	return !Pool || !Pool->IsActorInPool(Minion);
}

int32 USolarBossBrainComponent::GetAliveMinionCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Minion : Minions)
	{
		if (IsMinionAlive(Minion.Get())) ++Count;
	}
	return Count;
}

void USolarBossBrainComponent::PruneMinions()
{
	Minions.RemoveAll([this](const TWeakObjectPtr<AActor>& Minion) { return !IsMinionAlive(Minion.Get()); });
}

void USolarBossBrainComponent::CleanupMinions()
{
	if (Config->MinionCleanupOnDeath == ESolarBossMinionCleanup::Keep) return;

	PruneMinions();

	UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	const TArray<TWeakObjectPtr<AActor>> Remaining = Minions;
	Minions.Reset();

	for (const TWeakObjectPtr<AActor>& WeakMinion : Remaining)
	{
		AActor* Minion = WeakMinion.Get();
		if (!Minion) continue;

		if (const APawn* MinionPawn = Cast<APawn>(Minion); MinionPawn && MinionPawn->IsPlayerControlled()) continue;

		if (Config->MinionCleanupOnDeath == ESolarBossMinionCleanup::Kill && Minion->Implements<UDamageable>())
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

FBossDamageParams USolarBossBrainComponent::MakeDamageParams(const float Damage, const FGameplayTag& DamageTypeTag) const
{
	FBossDamageParams Params;
	Params.DamageEffectClass = Config->DamageEffectClass;
	Params.SetByCallerTag = Config->DamageSetByCallerTag;
	Params.Damage = Damage;
	Params.DamageTypeTag = DamageTypeTag;
	Params.CueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Damage.Default"), false);

	if (const FGameplayTag DodgeTag = FVelmaraGameplayTags::Get().State_Dodging; DodgeTag.IsValid())
	{
		Params.ImmunityTags.AddTag(DodgeTag);
	}

	return Params;
}

bool USolarBossBrainComponent::DamageSphere(const FVector& Center, const float Radius, const FBossDamageParams& Params, const bool bSingleTarget) const
{
	AActor* Owner = GetOwner();

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius), FCollisionQueryParams(SCENE_QUERY_STAT(SolarBossDamageSphere), false, Owner));

	TArray<AActor*> Damaged;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Damaged.Contains(Target)) continue;
		if (!UBossSystemLibrary::IsValidBossTarget(Owner, Target, Params.bOnlyPlayerControlled)) continue;
		if (UBossSystemLibrary::IsImmuneToBossHit(Target, Params.ImmunityTags)) continue;

		Damaged.Add(Target);

		const FVector TargetLocation = Target->GetActorLocation();
		const FHitResult Hit(Target, Overlap.GetComponent(), TargetLocation, (TargetLocation - Center).GetSafeNormal());
		UBossSystemLibrary::ApplyBossDamage(Owner, Owner, Target, Params, 1.f, Hit);

		if (bSingleTarget) break;
	}

	return Damaged.Num() > 0;
}

void USolarBossBrainComponent::ApplyShockwave() const
{
	if (Config->ShockwaveDamage <= 0.f || Config->ShockwaveRadius <= 0.f) return;

	DamageSphere(GetOwner()->GetActorLocation(), Config->ShockwaveRadius,
		MakeDamageParams(Config->ShockwaveDamage, FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false)), false);
}

int32 USolarBossBrainComponent::SelectMeleeAttack(const AActor* Target) const
{
	if (!IsReady() || !Target) return INDEX_NONE;

	const double Now = GetWorld()->GetTimeSeconds();
	const FVector ToTarget = Target->GetActorLocation() - GetOwner()->GetActorLocation();
	const float Distance = ToTarget.Size2D();
	const float Cos = FVector::DotProduct(GetOwner()->GetActorForwardVector().GetSafeNormal2D(), ToTarget.GetSafeNormal2D());
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Cos, -1.f, 1.f)));

	TArray<int32, TInlineAllocator<16>> Candidates;
	float TotalWeight = 0.f;

	for (int32 Index = 0; Index < Config->MeleeAttacks.Num(); ++Index)
	{
		const FSolarBossAttackConfig& Attack = Config->MeleeAttacks[Index];

		if (Attack.Weight <= 0.f || CompletedCycles < Attack.MinCycleIndex) continue;
		if (Distance < Attack.MinDistance || Distance > Attack.MaxDistance || Angle > Attack.MaxAngle) continue;
		if (AttackReadyTimes.IsValidIndex(Index) && Now < AttackReadyTimes[Index]) continue;

		Candidates.Add(Index);
		TotalWeight += Attack.Weight;
	}

	if (Candidates.Num() == 0) return INDEX_NONE;

	float Pick = FMath::FRandRange(0.f, TotalWeight);
	for (const int32 Index : Candidates)
	{
		Pick -= Config->MeleeAttacks[Index].Weight;
		if (Pick <= 0.f) return Index;
	}

	return Candidates.Last();
}

void USolarBossBrainComponent::CommitMeleeAttack(const int32 AttackIndex)
{
	if (!IsReady() || !Config->MeleeAttacks.IsValidIndex(AttackIndex)) return;

	if (AttackReadyTimes.Num() != Config->MeleeAttacks.Num())
	{
		AttackReadyTimes.SetNumZeroed(Config->MeleeAttacks.Num());
	}

	AttackReadyTimes[AttackIndex] = GetWorld()->GetTimeSeconds() + Config->MeleeAttacks[AttackIndex].Cooldown;
}

bool USolarBossBrainComponent::TryMeleeSphereHit(const int32 AttackIndex)
{
	if (!IsReady() || !Config->MeleeAttacks.IsValidIndex(AttackIndex)) return false;

	const FSolarBossAttackConfig& Attack = Config->MeleeAttacks[AttackIndex];
	const FVector Center = GetOwner()->GetActorTransform().TransformPositionNoScale(Attack.HitSphereOffset);

	return DamageSphere(Center, Attack.HitSphereRadius, MakeDamageParams(Attack.BaseDamage, Attack.DamageTypeTag), true);
}

void USolarBossBrainComponent::OpenPartialShafts(const int32 CycleIndex, AActor* Target)
{
	const FSolarBossCycleConfig& Cycle = Config->Cycles[CycleIndex];
	int32 Remaining = Cycle.LightShaftCount;
	TArray<FSolarBossShaftRequest> Requests;

	if (Cycle.bOpenShaftOnTarget && Target && Remaining > 0)
	{
		FSolarBossShaftRequest Request;
		if (ResolveShaftFromGround(Target->GetActorLocation(), Target, Request) && IsFarFromShafts(Request.Floor, Requests))
		{
			Requests.Add(Request);
			--Remaining;
		}
	}

	TArray<ABossCeilingBreakPoint*> Candidates;
	for (const TWeakObjectPtr<ABossCeilingBreakPoint>& Point : BreakPoints)
	{
		if (Point.IsValid() && Point->CanBreakAtStage(CycleIndex))
		{
			Candidates.Add(Point.Get());
		}
	}

	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}

	for (ABossCeilingBreakPoint* Point : Candidates)
	{
		if (Remaining <= 0) break;

		FSolarBossShaftRequest Request;
		if (!ResolveShaftFromCeiling(Point->GetActorLocation(), Request)) continue;
		if (!IsFarFromShafts(Request.Floor, Requests)) continue;

		Request.Point = Point;
		if (Point->ShaftRadiusOverride > 0.f)
		{
			Request.Radius = Point->ShaftRadiusOverride;
		}

		Requests.Add(Request);
		--Remaining;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	for (int32 Attempt = 0; NavSystem && Remaining > 0 && Attempt < SolarBossBrain::MaxPlacementAttempts * Cycle.LightShaftCount; ++Attempt)
	{
		FNavLocation NavLocation;
		if (!NavSystem->GetRandomReachablePointInRadius(ArenaOrigin, Config->ArenaRadius, NavLocation)) continue;
		if (!IsFarFromShafts(NavLocation.Location, Requests)) continue;

		FSolarBossShaftRequest Request;
		if (ResolveShaftFromGround(NavLocation.Location, nullptr, Request))
		{
			Requests.Add(Request);
			--Remaining;
		}
	}

	for (int32 Index = 0; Index < Requests.Num(); ++Index)
	{
		const float Delay = Config->ShaftStagger * Index;
		if (Delay <= 0.f)
		{
			OpenShaft(Requests[Index]);
			continue;
		}

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &USolarBossBrainComponent::OpenShaft, Requests[Index]), Delay, false);
	}
}

void USolarBossBrainComponent::CollapseWholeCeiling()
{
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SolarBossFullCollapse), false, GetOwner());
	int32 CollapseIndex = 0;

	for (const TWeakObjectPtr<ABossCeilingBreakPoint>& Point : BreakPoints)
	{
		if (!Point.IsValid()) continue;

		TraceParams.AddIgnoredActor(Point.Get());
		if (Point->IsBroken()) continue;

		const float Delay = Config->ShaftStagger * CollapseIndex++;
		if (Delay <= 0.f)
		{
			Point->Break();
			continue;
		}

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &USolarBossBrainComponent::BreakPointDeferred, Point), Delay, false);
	}

	FVector Floor = ArenaOrigin;
	FHitResult FloorHit;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, ArenaOrigin + FVector(0.f, 0.f, 200.f),
		ArenaOrigin - FVector(0.f, 0.f, Config->MaxCeilingTraceHeight), ECC_WorldStatic, TraceParams))
	{
		Floor.Z = FloorHit.ImpactPoint.Z;
	}

	const FVector Top = Floor + FVector(0.f, 0.f, Config->ShaftTopHeight);

	CloseAllShafts();

	if (ABossLightShaft* Shaft = AcquireShaft(Top))
	{
		Shaft->OpenFullArenaShaft(Top, Floor, Config->ArenaRadius, GetOwner(), MakeDamageParams(Config->ShockwaveDamage, FGameplayTag()));
		TrackShaft(Shaft);
	}
}

void USolarBossBrainComponent::BreakPointDeferred(const TWeakObjectPtr<ABossCeilingBreakPoint> Point)
{
	if (ABossCeilingBreakPoint* BreakPoint = Point.Get(); BreakPoint && !BreakPoint->IsBroken())
	{
		BreakPoint->Break();
	}
}

ABossLightShaft* USolarBossBrainComponent::AcquireShaft(const FVector& Location) const
{
	UClass* ShaftClass = Config->LightShaftClass.Get();
	if (!ShaftClass) return nullptr;

	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		if (ABossLightShaft* Pooled = Cast<ABossLightShaft>(Pool->GetActorFromPool(ShaftClass, FTransform(Location))))
		{
			return Pooled;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<ABossLightShaft>(ShaftClass, Location, FRotator::ZeroRotator, SpawnParams);
}

void USolarBossBrainComponent::OpenShaft(const FSolarBossShaftRequest Request)
{
	if (ABossLightShaft* Shaft = AcquireShaft(Request.Top))
	{
		Shaft->OpenShaft(GetOwner(), Request.Top, Request.Floor, Request.Radius,
			MakeDamageParams(Config->ShockwaveDamage, FGameplayTag()), Request.Point.Get());
		TrackShaft(Shaft);
	}
}

void USolarBossBrainComponent::TrackShaft(ABossLightShaft* Shaft)
{
	ActiveShafts.RemoveAll([](const TWeakObjectPtr<ABossLightShaft>& Existing) { return !Existing.IsValid() || Existing->IsHidden(); });
	ActiveShafts.AddUnique(Shaft);
}

void USolarBossBrainComponent::CloseAllShafts()
{
	for (const TWeakObjectPtr<ABossLightShaft>& Shaft : ActiveShafts)
	{
		if (Shaft.IsValid())
		{
			Shaft->CloseShaft();
		}
	}
	ActiveShafts.Reset();
}

bool USolarBossBrainComponent::ResolveShaftFromGround(const FVector& GroundPoint, const AActor* IgnoredActor, FSolarBossShaftRequest& OutRequest) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SolarBossCeilingTrace), false, GetOwner());
	if (IgnoredActor)
	{
		Params.AddIgnoredActor(IgnoredActor);

		TArray<AActor*> AttachedActors;
		IgnoredActor->GetAttachedActors(AttachedActors, true, true);
		Params.AddIgnoredActors(AttachedActors);
	}

	FVector Floor = GroundPoint;
	FHitResult FloorHit;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, GroundPoint + FVector(0.f, 0.f, 50.f), GroundPoint - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		Floor = FloorHit.ImpactPoint;
	}

	FVector Top = Floor + FVector(0.f, 0.f, Config->ShaftTopHeight);
	FHitResult CeilingHit;
	if (GetWorld()->LineTraceSingleByChannel(CeilingHit, Floor + FVector(0.f, 0.f, 120.f), Floor + FVector(0.f, 0.f, Config->MaxCeilingTraceHeight), ECC_Visibility, Params))
	{
		Top = FVector(CeilingHit.ImpactPoint) - FVector(0.f, 0.f, 5.f);
	}

	if (Top.Z - Floor.Z < SolarBossBrain::MinShaftHeight) return false;

	OutRequest.Top = Top;
	OutRequest.Floor = Floor;
	OutRequest.Radius = Config->ShaftRadius;
	return true;
}

bool USolarBossBrainComponent::ResolveShaftFromCeiling(const FVector& CeilingPoint, FSolarBossShaftRequest& OutRequest) const
{
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(SolarBossCeilingTrace), false, GetOwner());
	const FVector Start = CeilingPoint - FVector(0.f, 0.f, 20.f);

	FVector Floor = CeilingPoint - FVector(0.f, 0.f, Config->ShaftTopHeight);
	FHitResult FloorHit;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, Start, Start - FVector(0.f, 0.f, Config->MaxCeilingTraceHeight), ECC_Visibility, Params))
	{
		Floor = FloorHit.ImpactPoint;
	}

	if (CeilingPoint.Z - Floor.Z < SolarBossBrain::MinShaftHeight) return false;

	OutRequest.Top = CeilingPoint;
	OutRequest.Floor = Floor;
	OutRequest.Radius = Config->ShaftRadius;
	return true;
}

bool USolarBossBrainComponent::IsFarFromShafts(const FVector& Point, const TArray<FSolarBossShaftRequest>& Requests) const
{
	for (const FSolarBossShaftRequest& Request : Requests)
	{
		if (FVector::Dist2D(Request.Floor, Point) < Config->MinShaftSeparation) return false;
	}

	for (const TWeakObjectPtr<ABossLightShaft>& Shaft : ActiveShafts)
	{
		if (Shaft.IsValid() && !Shaft->IsHidden() && FVector::Dist2D(Shaft->GetFloorLocation(), Point) < Config->MinShaftSeparation) return false;
	}

	return true;
}

void USolarBossBrainComponent::HandleDeath()
{
	if (bDead) return;

	bDead = true;
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	PendingCycles.Reset();
	PendingSpawns = 0;
	ActiveCycleIndex = INDEX_NONE;
	bCycleBreakArmed = false;
	bWantsDefensiveDodge = false;
	bNeedsEmergencySummon = false;
	bManualSuperArmor = false;
	bTimedSuperArmor = false;

	SetMode(ESolarBossMode::Dead);
	SignalEncounterListeners(false);
	CleanupMinions();

	if (Config->bCloseShaftsOnDeath)
	{
		CloseAllShafts();
	}

	UpdateHealthBar();
	if (Config->HealthBarHideDelay > 0.f)
	{
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &USolarBossBrainComponent::HideHealthBar, Config->HealthBarHideDelay, false);
	}
	else
	{
		HideHealthBar();
	}

	if (AAIController* Controller = Cast<AAIController>(OwnerEntity->GetController()))
	{
		if (UBrainComponent* BrainLogic = Controller->GetBrainComponent())
		{
			BrainLogic->StopLogic(TEXT("SolarBossDeath"));
		}
		Controller->StopMovement();
	}
}
