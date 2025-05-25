// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Paladin.h"
#include "PaladinBoss.generated.h"

UCLASS()
class TESISUE_API APaladinBoss : public APaladin
{
	GENERATED_BODY()

protected:
	APaladinBoss();

	void BeginPlay() override;

	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (DisplayName = "Initial Minion Pool Size Per Boss"))
	int32 InitialMinionPoolSize = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<USceneComponent*> SpawnPoints;

	void DirectionalHitReact(const FVector& ImpactPoint, UAnimMontage* HitReactAnimMontage) override;

	bool IsLaunchable_Implementation(ACharacter* DamageCauser) override;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AEnemy> MinionToSpawnClass;
	
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* InvokeMontage;

	TArray<AEnemy*> Minions;
	FTimerHandle InvokeTimer;

	UFUNCTION(BlueprintCallable)
	void TryToInvoke();
	
	UFUNCTION(BlueprintCallable)
	bool CanInvoke();

	UFUNCTION()
	void Invoke();

	UFUNCTION()
	void HandleMinionDeactivated(AEnemy* DeactivatedMinion);	

	UFUNCTION()
	void NotifyDamageTakenToBlackboard();
};
