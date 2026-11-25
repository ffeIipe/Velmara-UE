#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Features/SolarBoss/Data/SolarBossTypes.h"
#include "SolarBossConfigDataAsset.generated.h"

class ABossLightShaft;
class UCameraShakeBase;
class UGameplayEffect;
class UNiagaraSystem;
class UPaladinBossHealthBar;
class USoundBase;

UCLASS(BlueprintType)
class TESISUE_API USolarBossConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USolarBossConfigDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycles")
	TArray<FSolarBossCycleConfig> Cycles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacks")
	TArray<FSolarBossAttackConfig> MeleeAttacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0"))
	float MaxHealthOverride = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0"))
	float BullyWalkSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0"))
	float EvadeWalkSpeed = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0"))
	float AggroRadius = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TSubclassOf<UPaladinBossHealthBar> HealthBarClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0"))
	float HealthBarHideDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FGameplayTag DamageSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock")
	bool bAllowHitReactInBullyMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock")
	FGameplayTagContainer HitReactAbilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock", meta = (ClampMin = "1"))
	int32 ConsecutiveHitsForDefensiveDodge = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock", meta = (ClampMin = "0"))
	float ConsecutiveHitWindow = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock", meta = (ClampMin = "0"))
	float DefensiveDodgeCooldown = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact & Anti-Stunlock")
	bool bTriggerCycleEscapeImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeBackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.1"))
	float DodgePlayRate = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeDistance = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0.05"))
	float DodgeDurationFallback = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeSuperArmorDuration = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "0"))
	float EvadePreferredDistance = 1250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "0"))
	float EvadeMinSafeDistance = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "0"))
	float EvadePanicDodgeRadius = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "0"))
	float EvadeReactiveDodgeCooldown = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting")
	FVector2D EvadeStrafeAngles = FVector2D(30.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "0"))
	float EvadeRepathInterval = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evade & Kiting", meta = (ClampMin = "1"))
	int32 EvadeSearchSamples = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	TObjectPtr<UAnimMontage> CeilingBreakMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0", ClampMax = "1"))
	float CeilingBreakEventTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0.1"))
	float CeilingBreakPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0.05"))
	float CeilingBreakFallbackDuration = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float ShockwaveRadius = 420.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float ShockwaveDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	TSubclassOf<ABossLightShaft> LightShaftClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "50"))
	float ShaftRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float ShaftStagger = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float MinShaftSeparation = 480.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float ArenaRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "150"))
	float ShaftTopHeight = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "150"))
	float MaxCeilingTraceHeight = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	int32 PrewarmLightShafts = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	bool bCloseShaftsOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	TSubclassOf<UCameraShakeBase> BreakCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight", meta = (ClampMin = "0"))
	float BreakCameraShakeRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	TObjectPtr<USoundBase> BreakSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Break & Sunlight")
	TObjectPtr<UNiagaraSystem> BreakEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	TArray<TSubclassOf<AActor>> DefaultMinionClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	TObjectPtr<UAnimMontage> SummonMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0", ClampMax = "1"))
	float SummonEventTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0.1"))
	float SummonPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0.05"))
	float SummonFallbackDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	int32 MaxAliveMinions = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	int32 MaxAliveMinionsFinalCycle = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	float MinSpawnRadius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	float MaxSpawnRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	float SpawnDelay = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	float SpawnStagger = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	TObjectPtr<UNiagaraSystem> SpawnEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	bool bForceMinionsPossessable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning", meta = (ClampMin = "0"))
	int32 PrewarmMinionsPerClass = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summoning")
	ESolarBossMinionCleanup MinionCleanupOnDeath = ESolarBossMinionCleanup::Kill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Final Collapse Anti-Softlock", meta = (ClampMin = "0"))
	int32 MinGuaranteedMinionsFinalCycle = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Final Collapse Anti-Softlock", meta = (ClampMin = "0"))
	float EmergencySummonCooldown = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Final Collapse Anti-Softlock", meta = (ClampMin = "0"))
	float EmergencyFallbackDelay = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession Interaction", meta = (ClampMin = "1"))
	float PossessedMinionDamageMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession Interaction", meta = (ClampMin = "0"))
	float PostPossessionSunGraceDuration = 1.2f;
};
