#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/OrbPoolingInterface.h"
#include "OrbBase.generated.h"

class UNiagaraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrbArrivedSignature, AActor*, Target);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOrbArrivedNative, AActor*);

UCLASS()
class TESISUE_API AOrbBase : public AActor, public IOrbPoolingInterface
{
	GENERATED_BODY()

public:
	AOrbBase();

	UFUNCTION(BlueprintCallable, Category = "Orb")
	void LaunchToTarget(AActor* Target);

	virtual void ActivateOrb_Implementation(const FVector& Location, const FRotator& Rotation) override;
	virtual void DeactivateOrb_Implementation() override;

	virtual void Tick(float DeltaSeconds) override;

	FOnOrbArrivedNative OnArrivedNative;

	UPROPERTY(BlueprintAssignable, Category = "Orb")
	FOnOrbArrivedSignature OnArrived;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Effect", meta = (EditCondition = "EffectClass != nullptr"))
	FGameplayTag MagnitudeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orb | Effect", meta = (EditCondition = "EffectClass != nullptr"))
	float Magnitude = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "cm/s"))
	float BurstSpeed = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "cm/s"))
	float BurstUpSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst")
	float BurstDrag = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "deg/s"))
	float ArcTurnRate = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "s"))
	float HoverTimeMin = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Burst", meta = (Units = "s"))
	float HoverTimeMax = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm/s^2"))
	float SeekAcceleration = 7000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm/s"))
	float MaxSpeed = 3500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek")
	float SteerRate = 7.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "cm"))
	float ArriveDistance = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek")
	FVector TargetOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Orb | Seek", meta = (Units = "s"))
	float MaxLifetime = 8.f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Orb")
	void ReceiveArrived(AActor* Target);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orb")
	TObjectPtr<UNiagaraComponent> Niagara;

private:
	void Arrive();
	void ReturnToPool();
	void ApplyEffect(AActor* Target) const;

	TWeakObjectPtr<AActor> TargetActor;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float HoverTime = 1.f;
	float ArcSign = 1.f;
};
