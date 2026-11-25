#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TotemChargeComponent.generated.h"

class AOrbBase;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemEnemyAbsorbedSignature, AActor*, DeadEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTotemChargeChangedSignature, int32, CurrentKills, int32, KillsToCharge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTotemChargeStateSignature);

UCLASS(ClassGroup=(Totem), meta=(BlueprintSpawnableComponent))
class TESISUE_API UTotemChargeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTotemChargeComponent();

	UFUNCTION(BlueprintPure, Category = "Totem | Charge", meta = (DefaultToSelf = "Actor"))
	static bool CanActorBePossessed(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	bool RequiresCharge() const { return bRequiresCharge; }

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	bool IsCharged() const;

	UFUNCTION(BlueprintPure, Category = "Totem | Possession")
	bool IsActivated() const { return bActivated && IsCharged(); }

	UFUNCTION(BlueprintCallable, Category = "Totem | Possession")
	void ActivatePossession(UMaterialInterface* OverlayMaterial);

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	int32 GetCurrentKills() const { return CurrentKills; }

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	int32 GetKillsToCharge() const { return KillsToCharge; }

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	float GetChargePercent() const;

	UFUNCTION(BlueprintPure, Category = "Totem | Charge")
	float GetChargeRadius() const { return ChargeRadius; }

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	bool TryAbsorbDeath(AActor* DeadActor);

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void AddCharge(int32 Kills = 1);

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void FillCharge();

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void ConsumeCharge();

	UFUNCTION(BlueprintCallable, Category = "Totem | Charge")
	void SetRequiresCharge(bool bNewRequiresCharge);

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemEnemyAbsorbedSignature OnEnemyAbsorbed;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeChangedSignature OnChargeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeStateSignature OnFullyCharged;

	UPROPERTY(BlueprintAssignable, Category = "Totem | Events")
	FOnTotemChargeStateSignature OnChargeConsumed;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge")
	bool bRequiresCharge = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (ClampMin = "1", EditCondition = "bRequiresCharge"))
	int32 KillsToCharge = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (ClampMin = "0", Units = "cm", EditCondition = "bRequiresCharge"))
	float ChargeRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (EditCondition = "bRequiresCharge"))
	bool bStartCharged = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Charge", meta = (EditCondition = "bRequiresCharge"))
	bool bOnlyCountEnemies = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Visuals", meta = (EditCondition = "bRequiresCharge"))
	TSubclassOf<AOrbBase> AbsorbOrbClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Possession")
	bool bIgnorePossessionTraceWhileUncharged = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Possession", meta = (EditCondition = "bIgnorePossessionTraceWhileUncharged"))
	TEnumAsByte<ECollisionChannel> PossessionTraceChannel = ECC_GameTraceChannel6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Totem | Visuals")
	FName ChargeMaterialParameter = FName("ChargePercent");

private:
	void SetCurrentKills(int32 NewKills);

	void NotifyChargeChanged(bool bWasCharged);

	bool IsOwnerPlayerControlled() const;

	bool IsEnemyOfPlayer(const AActor* DeadActor) const;

	void RefreshPossessionTraceResponse();

	void RefreshChargeMaterials() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Totem | Charge")
	int32 CurrentKills = 0;

	int32 PendingOrbs = 0;

	bool bActivated = false;

	TMap<TWeakObjectPtr<UPrimitiveComponent>, TEnumAsByte<ECollisionResponse>> CachedTraceResponses;
};
