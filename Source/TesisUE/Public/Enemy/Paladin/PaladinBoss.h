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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USpectralTrapComponent* SpectralTrapComponent2;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* AuraMeshComponent;

	void DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage, const float DamageReceived) override;

	//void GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived) override;

	UPROPERTY(EditDefaultsOnly, Category = "Attacks | Flood")
	float FloodDamage;

	bool bCanFloodDamage;

	FTimerHandle EnableFloodDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* FloodToRaise;

	UFUNCTION(BlueprintImplementableEvent)
	void RaiseFlood();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attacks | Aura")
	float AuraRadius = 4.f;

private:
	UPROPERTY(EditAnywhere, Category = "Attacks | Spawning")
	TSubclassOf<AEnemy> MinionToSpawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* InvokeMontage;

	UPROPERTY(VisibleAnywhere)
	TArray<AEnemy*> Minions;

	FTimerHandle InvokeTimer;

	UFUNCTION(BlueprintCallable)
	void TryToInvoke();
	
	UFUNCTION(BlueprintCallable)
	bool CanInvoke();

	void Invoke();

	UFUNCTION(BlueprintCallable)
	void FloodAttack();

	UFUNCTION() //el fkn panzas del UFUNCTION tiene que estar si o si, en caso de ser bindeado a un multicast delegate
	void HandleMinionDeactivated(AEnemy* DeactivatedMinion);	

	void HandleFloodDamage();

	class UBlackboardComponent* BBComponent;
};
