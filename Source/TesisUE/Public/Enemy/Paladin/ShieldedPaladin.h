// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Paladin.h"
#include "ShieldedPaladin.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API AShieldedPaladin : public APaladin
{
	GENERATED_BODY()

public:
	AShieldedPaladin();

	UFUNCTION(BlueprintCallable)
	void Disarm();
	
	float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	UPROPERTY(BlueprintReadWrite)
	bool bIsDisarmed = false;

protected:
	void BeginPlay() override;

	bool IsLaunchable_Implementation() override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ShieldMesh;
};
