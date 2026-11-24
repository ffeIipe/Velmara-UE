#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "BossTypes.generated.h"

class UAnimMontage;
class UGameplayEffect;
class USoundBase;

UENUM(BlueprintType)
enum class EBossAttackRange : uint8
{
	Short	UMETA(DisplayName = "Corto"),
	Medium	UMETA(DisplayName = "Mediano"),
	Long	UMETA(DisplayName = "Largo")
};

UENUM(BlueprintType)
enum class EBossHitShape : uint8
{
	Weapon,
	Sphere,
	Projectile
};

UENUM(BlueprintType)
enum class EBossBrainState : uint8
{
	Inactive,
	Chasing,
	Attacking,
	Recovering,
	Summoning,
	BreakingCeiling,
	Evading,
	Dead,
	Staggered UMETA(DisplayName = "Staggered")
};

USTRUCT(BlueprintType)
struct FBossDamageParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag CueTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTagContainer ImmunityTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bOnlyPlayerControlled = true;
};

USTRUCT(BlueprintType)
struct FBossHitWindow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0", ClampMax = "1"))
	float StartTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0", ClampMax = "1"))
	float EndTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
	EBossHitShape Shape = EBossHitShape::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Sphere", EditConditionHides))
	FVector SphereOffset = FVector(150.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Sphere", EditConditionHides, ClampMin = "1"))
	float SphereRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Projectile", EditConditionHides, ClampMin = "1"))
	int32 ProjectileCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Projectile", EditConditionHides, ClampMin = "0", ClampMax = "180"))
	float ProjectileSpreadAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
	bool bAllowMultipleHits = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "bAllowMultipleHits", ClampMin = "0.05"))
	float ReHitDelay = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
	FGameplayTag DamageTypeTag;
};

USTRUCT(BlueprintType)
struct FBossAttackDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	FName AttackName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	EBossAttackRange Range = EBossAttackRange::Short;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MaxDistance = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "180"))
	float MinAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxAngle = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Cooldown = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	int32 MinStage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0"))
	float BaseDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TArray<FBossHitWindow> HitWindows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", ClampMax = "1"))
	float TrackingEndTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float TrackingSpeed = 270.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	bool bShowGroundTelegraph = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (EditCondition = "bShowGroundTelegraph"))
	float TelegraphRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (EditCondition = "bShowGroundTelegraph"))
	FVector TelegraphOffset = FVector(150.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<USoundBase> TelegraphSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (ClampMin = "0"))
	float RecoveryTime = 0.8f;
};

USTRUCT(BlueprintType)
struct FBossCeilingStage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0"))
	int32 ShaftCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bOpenShaftOnTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bSummonMinions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (EditCondition = "bSummonMinions", ClampMin = "0"))
	int32 MinionCountOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling")
	bool bFullArenaCollapse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling")
	bool bActivateSolarShield = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Ceiling", meta = (ClampMin = "0.0"))
	float SolarShieldAmountOverride = 0.f;
};
