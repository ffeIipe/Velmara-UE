#pragma once

#include "CoreMinimal.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "GameFramework/Actor.h"
#include "GenericPool/Interfaces/PoolableActor.h"
#include "BossProjectile.generated.h"

class AEntity;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;

UCLASS()
class TESISUE_API ABossProjectile : public AActor, public IPoolableActor
{
	GENERATED_BODY()

public:
	ABossProjectile();

	UFUNCTION(BlueprintCallable, Category = "Boss|Projectile")
	void Launch(AActor* InInstigator, const FVector& Direction, float Speed, float Lifetime, const FBossDamageParams& InDamageParams);

	UFUNCTION(BlueprintCallable, Category = "Boss|Projectile")
	void SetHomingTarget(AActor* TargetActor, float HomingAcceleration);

	UFUNCTION(BlueprintCallable, Category = "Boss|Projectile")
	void ClearHomingTarget();

	UFUNCTION(BlueprintCallable, Category = "Boss|Projectile")
	void SetPierceTargets(bool bShouldPierce);

	UFUNCTION(BlueprintCallable, Category = "Boss|Projectile")
	void SetCollisionRadius(float Radius);

	virtual void OnPoolActivate_Implementation() override;

	virtual void OnPoolDeactivate_Implementation() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Projectile")
	void OnLaunched();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Projectile")
	void OnImpact(const FHitResult& Hit, bool bHitTarget);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> Effect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Projectile")
	bool bPierceTargets = false;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleStop(const FHitResult& ImpactResult);

	UFUNCTION()
	void HandleHomingTargetDead(AEntity* DeadEntity);

	void ConfigureMoveIgnores(AActor* InInstigator);

	void Expire();

	void PlayImpactFeedback(const FVector& Location) const;

	FBossDamageParams DamageParams;

	TWeakObjectPtr<AActor> BossInstigator;

	TWeakObjectPtr<AEntity> HomingEntity;

	TArray<TWeakObjectPtr<AActor>> HandledActors;

	FTimerHandle LifetimeTimer;

	float DefaultCollisionRadius = 55.f;

	bool DefaultPierceTargets = false;

	bool bLaunched = false;
};
