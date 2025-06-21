#pragma once

#include "CoreMinimal.h"
#include "Enemy/Paladin.h"
#include "PaladinBoss.generated.h"

UCLASS()
class TESISUE_API APaladinBoss : public APaladin
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ResetKeyBool(float Duration, struct FBlackboardKeySelector Key, bool SetBool, APawn* TargetPawn);

protected:
	APaladinBoss();

	void BeginPlay() override;

	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, Category = "Attacks | Spawning")
	int32 InitialMinionPoolSize = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attacks | Spawning")
	TArray<class USpawnPointComponent*> SpawnPoints;

	UPROPERTY(VisibleAnywhere)
	class USpectralTrapComponent* SpectralTrapComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AuraMeshComponent;

	void DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage, const float DamageReceived) override;

	bool IsLaunchable_Implementation(ACharacter* DamageCauser) override;

	void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;

	UPROPERTY(EditDefaultsOnly, Category = "Attacks | Flood")
	float FloodDamage;

	bool bCanFloodDamage;

	FTimerHandle EnableFloodDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* FloodToRaise;

	UFUNCTION(BlueprintImplementableEvent)
	void RaiseFlood();

private:
	UPROPERTY(EditAnywhere, Category = "Attacks | Spawning")
	TSubclassOf<AEnemy> MinionToSpawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* InvokeMontage;

	TArray<AEnemy*> Minions;

	FTimerHandle InvokeTimer;

	UFUNCTION(BlueprintCallable)
	void TryToInvoke();
	
	UFUNCTION(BlueprintCallable)
	bool CanInvoke();

	void Invoke();

	UFUNCTION(BlueprintCallable)
	void FloodAttack();

	/*UFUNCTION(BlueprintCallable)
	void ApplyFloodDamage(AActor* PlayerRef);*/

	void HandleMinionDeactivated(AEnemy* DeactivatedMinion);	

	void HandleFloodDamage();
};
