#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "BossEnemy.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class UBossCeilingBreakComponent;
class UBossCombatComponent;
class UBossSummonComponent;
class UPaladinBossHealthBar;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossEncounterEvent);

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
	AActor* GetCombatTarget() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossSummonComponent* GetSummonComponent() const { return SummonComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossCeilingBreakComponent* GetCeilingBreakComponent() const { return CeilingBreakComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float MaxHealthOverride = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float WalkSpeed = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stats", meta = (ClampMin = "0"))
	float WalkSpeedBonusPerStage = 40.f;

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
	float EvadeChance = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "1"))
	int32 EvadeHitThreshold = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "0.1"))
	float EvadeHitWindow = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI|Evade", meta = (ClampMin = "0"))
	float EvadeCooldown = 7.f;

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

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Encounter Started"))
	void BP_OnEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Boss Defeated"))
	void BP_OnBossDefeated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss", meta = (DisplayName = "On Stage Changed"))
	void BP_OnStageChanged(int32 NewStage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossSummonComponent> SummonComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossCeilingBreakComponent> CeilingBreakComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

private:
	void SetupDefaultAssets();

	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void HandleStageChanged(int32 NewStage);

	void UpdateHealthBar() const;

	void HideHealthBar();

	UPROPERTY(Transient)
	TObjectPtr<UPaladinBossHealthBar> HealthBarWidget;

	FTimerHandle HideHealthBarTimer;

	bool bEncounterStarted = false;

	bool bDefeated = false;
};
