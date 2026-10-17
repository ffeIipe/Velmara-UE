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
	// Barre el arma entre frames: solo golpea lo que el arma atraviesa de verdad.
	Weapon,
	// Esfera relativa al boss (giros, golpes al piso).
	Sphere,
	// Dispara proyectiles al empezar la ventana (EndTime se ignora).
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
	Dead
};

// Parámetros comunes para aplicar daño con el GE del proyecto (GE_Damage: SetByCaller "Damage" sobre DamageIncoming).
USTRUCT(BlueprintType)
struct FBossDamageParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag SetByCallerTag;

	// Positivo = daño (DamageIncoming).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage = 10.f;

	// Tag de tipo de daño que recibe el ReceiveDamage del objetivo (reacciones de golpe).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag CueTag;

	// Si el objetivo tiene alguno de estos tags, el golpe se esquiva (i-frames del dodge).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTagContainer ImmunityTags;

	// Solo daña pawns controlados por un jugador (Velmara o el cuerpo que esté poseyendo). Nunca a sus propios invocados.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bOnlyPlayerControlled = true;
};

USTRUCT(BlueprintType)
struct FBossHitWindow
{
	GENERATED_BODY()

	// Tiempo normalizado (0-1) del montaje. Se ignora si la ventana viene de un AnimNotifyState "Boss Hit Window".
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0", ClampMax = "1"))
	float StartTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0", ClampMax = "1"))
	float EndTime = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
	EBossHitShape Shape = EBossHitShape::Weapon;

	// Offset local respecto al boss (X = adelante, Z = arriba desde el centro de la cápsula).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Sphere", EditConditionHides))
	FVector SphereOffset = FVector(150.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Sphere", EditConditionHides, ClampMin = "1"))
	float SphereRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Projectile", EditConditionHides, ClampMin = "1"))
	int32 ProjectileCount = 1;

	// Apertura total del abanico de proyectiles, en grados.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "Shape == EBossHitShape::Projectile", EditConditionHides, ClampMin = "0", ClampMax = "180"))
	float ProjectileSpreadAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	// Permite que un mismo objetivo reciba varios golpes en la ventana (combos), separados por ReHitDelay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
	bool bAllowMultipleHits = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window", meta = (EditCondition = "bAllowMultipleHits", ClampMin = "0.05"))
	float ReHitDelay = 0.35f;

	// Vacío = usa el DamageTypeTag del ataque.
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

	// Distancia horizontal al objetivo (centro a centro) en la que el ataque puede elegirse.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float MaxDistance = 350.f;

	// Ángulo entre el frente del boss y el objetivo. MinAngle >= 100 = ataques que giran hacia atrás.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "180"))
	float MinAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxAngle = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	float Cooldown = 4.f;

	// Etapa mínima (0 = desde el inicio). La etapa sube cada vez que el boss rompe el techo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0"))
	int32 MinStage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0"))
	float BaseDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TArray<FBossHitWindow> HitWindows;

	// Hasta este tiempo normalizado el boss sigue girando hacia el objetivo. Después queda comprometido: ahí se esquiva.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0", ClampMax = "1"))
	float TrackingEndTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (ClampMin = "0"))
	float TrackingSpeed = 270.f;

	// Decal en el piso marcando la zona de impacto hasta el primer golpe (usa GroundTelegraphMaterial del componente).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	bool bShowGroundTelegraph = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (EditCondition = "bShowGroundTelegraph"))
	float TelegraphRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge", meta = (EditCondition = "bShowGroundTelegraph"))
	FVector TelegraphOffset = FVector(150.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dodge")
	TObjectPtr<USoundBase> TelegraphSound = nullptr;

	// Pausa después del ataque antes de la próxima decisión (se reduce con cada etapa).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (ClampMin = "0"))
	float RecoveryTime = 0.8f;

	// Escala el root motion para que el desplazamiento llegue hasta el objetivo (útil en ataques que acortan distancia).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	bool bScaleRootMotionToTarget = false;

	// Distancia que recorre el root motion del montaje a escala 1.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion", meta = (EditCondition = "bScaleRootMotionToTarget", ClampMin = "1"))
	float RootMotionNominalDistance = 500.f;
};

USTRUCT(BlueprintType)
struct FBossCeilingStage
{
	GENERATED_BODY()

	// Porcentaje de vida (0-1) que dispara la etapa.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "0"))
	int32 ShaftCount = 2;

	// Uno de los huecos se abre sobre el jugador (lo obliga a moverse durante el telegraph).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bOpenShaftOnTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bSummonMinions = true;

	// 0 = usa MinionsPerSummon del componente de invocación.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (EditCondition = "bSummonMinions", ClampMin = "0"))
	int32 MinionCountOverride = 0;
};
