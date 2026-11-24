#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "BossEnemy.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UAnimSequenceBase;
class UBossCeilingBreakComponent;
class UBossCombatComponent;
class UBossSummonComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;
class UPaladinBossHealthBar;
class USoundBase;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossEncounterEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSolarShieldActivated, float, ShieldAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossSolarShieldBroken, AActor*, Breaker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBossHitByPossessedMinion, AActor*, Minion, float, ShieldDamage, float, HealthDamage);

UCLASS()
class TESISUE_API ABossEnemy : public AEntity
{
	GENERATED_BODY()

public:
	ABossEnemy();

	virtual void PossessedBy(AController* NewController) override;

	virtual void ReceiveDamage_Implementation(FGameplayEventData DamagePayload) override;

	virtual void MortalDamage_Implementation(FGameplayEventData DeathPayload) override;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	void StartEncounter();

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsEncounterStarted() const { return bEncounterStarted; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsDefeated() const { return bDefeated; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	float GetShieldPercent() const;

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsSolarShieldActive() const { return bSolarShieldActive; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Solar Shield")
	void ActivateSolarShield(float ShieldAmount = -1.f);

	bool IsPossessedMinionAttacker(const AActor* AttackerActor, const AActor* InstigatorActor, const UAbilitySystemComponent* SourceASC) const;

	void ResolveIncomingDamage(float RawDamage, float CurrentShield, const AActor* AttackerActor, const AActor* InstigatorActor,
		const UAbilitySystemComponent* SourceASC, float& OutDamageToShield, float& OutDamageToHealth, bool& bOutHitByPossessedMinion) const;

	void NotifyHitByPossessedMinion(AActor* MinionActor, float ShieldDamage, float HealthDamage);

	void HandleSolarShieldBroken(AActor* BreakerActor);

	UFUNCTION(BlueprintPure, Category = "Boss")
	AActor* GetCombatTarget() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossSummonComponent* GetSummonComponent() const { return SummonComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossCeilingBreakComponent* GetCeilingBreakComponent() const { return CeilingBreakComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UNiagaraComponent* GetSolarShieldAuraComponent() const { return SolarShieldAuraComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float MaxHealthOverride = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float WalkSpeed = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float WalkSpeedBonusPerStage = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI")
	bool bAutoStartEncounter = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI", meta = (EditCondition = "bAutoStartEncounter", ClampMin = "0"))
	float AggroRadius = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI", meta = (ClampMin = "0"))
	float ChaseAcceptanceRadius = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI", meta = (ClampMin = "0.02"))
	float DecisionInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI", meta = (ClampMin = "0"))
	float FacingSpeed = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade")
	TArray<TObjectPtr<UAnimMontage>> EvadeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "0", ClampMax = "1"))
	float EvadeChance = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "1"))
	int32 EvadeHitThreshold = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "0.1"))
	float EvadeHitWindow = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "0"))
	float EvadeCooldown = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float Stage3ShieldAmount = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float PossessedMinionShieldMultiplier = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float UnpossessedSolarShieldMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float PossessedMinionHealthMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float PossessedMinionHitEnergyGain = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float PossessedMinionEnergyGainCooldown = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float SolarShieldBreakEnergyBonus = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield", meta = (ClampMin = "0.0"))
	float SolarShieldBreakStaggerDuration = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	TObjectPtr<UAnimMontage> SolarShieldBreakStaggerMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	TObjectPtr<UNiagaraSystem> SolarShieldAuraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	TObjectPtr<UMaterialInterface> SolarShieldOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	bool bApplyShieldOverlayMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	TObjectPtr<USoundBase> SolarShieldActivateSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Solar Shield")
	TObjectPtr<USoundBase> SolarShieldBreakSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|UI")
	TSubclassOf<UPaladinBossHealthBar> HealthBarClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|UI", meta = (ClampMin = "0"))
	float HideHealthBarDelay = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death")
	TObjectPtr<UAnimSequenceBase> DeathAnimation;

	UPROPERTY(BlueprintAssignable, Category = "Boss")
	FOnBossEncounterEvent OnEncounterStarted;

	UPROPERTY(BlueprintAssignable, Category = "Boss")
	FOnBossEncounterEvent OnBossDefeated;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossSolarShieldActivated OnSolarShieldActivated;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossSolarShieldBroken OnSolarShieldBroken;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Events")
	FOnBossHitByPossessedMinion OnHitByPossessedMinion;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Encounter Started"))
	void BP_OnEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Boss Defeated"))
	void BP_OnBossDefeated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Stage Changed"))
	void BP_OnStageChanged(int32 NewStage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Events")
	void BP_OnSolarShieldActivated(float ShieldAmount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Events")
	void BP_OnSolarShieldBroken(AActor* Breaker);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Events")
	void BP_OnHitByPossessedMinion(AActor* Minion, float ShieldDamage, float HealthDamage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossSummonComponent> SummonComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossCeilingBreakComponent> CeilingBreakComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Solar Shield")
	TObjectPtr<UNiagaraComponent> SolarShieldAuraComponent;

private:
	void SetupDefaultAssets();

	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	void HandleHealthBarAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleStageChanged(int32 NewStage);

	void UpdateHealthBar() const;

	void HideHealthBar();

	UPROPERTY(Transient)
	TObjectPtr<UPaladinBossHealthBar> HealthBarWidget;

	FTimerHandle HideHealthBarTimer;

	bool bEncounterStarted = false;

	bool bDefeated = false;

	bool bSolarShieldActive = false;

	double LastPossessedMinionEnergyGainTime = -1000.0;
};
