#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "SolarBossTypes.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class ESolarBossMode : uint8
{
	Inactive,
	BullyMelee,
	CycleBreakEscape,
	EvadeWithMinions,
	Dead
};

UENUM(BlueprintType)
enum class ESolarBossDodgeDirection : uint8
{
	AwayFromTarget,
	StrafeLeft,
	StrafeRight,
	BestNavigable
};

UENUM(BlueprintType)
enum class ESolarBossDodgeMode : uint8
{
	EscapeCycle,
	DefensiveAntiStunlock,
	ReactiveEvade
};

UENUM(BlueprintType)
enum class ESolarBossMinionCleanup : uint8
{
	Kill,
	ReturnToPool,
	Keep
};

USTRUCT(BlueprintType)
struct FSolarBossCycleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	bool bFullCeilingCollapse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "0"))
	int32 LightShaftCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	bool bOpenShaftOnTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "0"))
	int32 MinionCountToSpawn = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	TArray<TSubclassOf<AActor>> MinionClassesOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "0"))
	int32 MinMinionsToStayEvading = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "0"))
	int32 MaxAliveMinionsOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	float WalkSpeedBonus = 0.f;
};

USTRUCT(BlueprintType)
struct FSolarBossAttackConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName AttackName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FGameplayTag AbilityTriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.05"))
	float FallbackDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MaxDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxAngle = 65.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Cooldown = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	int32 MinCycleIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0"))
	float BaseDamage = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	bool bUseEquippedWeaponHitbox = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0", ClampMax = "1"))
	float HitStartFraction = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "0", ClampMax = "1"))
	float HitEndFraction = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit")
	FVector HitSphereOffset = FVector(120.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit", meta = (ClampMin = "1"))
	float HitSphereRadius = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tracking", meta = (ClampMin = "0", ClampMax = "1"))
	float TrackingEndFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tracking", meta = (ClampMin = "0"))
	float TrackingSpeed = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (ClampMin = "0"))
	float RecoveryTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery")
	bool bGrantSuperArmorDuringAttack = false;
};
