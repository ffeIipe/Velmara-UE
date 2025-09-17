// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CombatStrategy.generated.h"

class UTargetingComponent;
class UCharacterMovementComponent;
class UExtraMovementComponent;
class UStrategyData;
class UCombatComponent;
class IWeaponInterface;
class IWeaponProvider;

USTRUCT(BlueprintType)
struct FStrategyProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DoubleJumpForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDodgeDistance = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAbilityDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAbilityRadius = 50.f;
	/*
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionHeavyAttackCost;
	
	UPROPERTY(EditDefaultsOnly, Category = "Energy | OnPossession")
	float PossessionDamage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement | OnPossession")
	float PossessionMaxWalkSpeed = 700.f;*/
};

USTRUCT(BlueprintType)
struct FStrategyMontages
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* JumpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* DoubleJumpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* DodgeMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* FinisherMontage;
};

UCLASS(Abstract)
class TESISUE_API UCombatStrategy : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void Strategy_UseFirstAttack(const bool bIsInAir, TScriptInterface<IWeaponInterface> CurrentWeapon){}
	virtual void Strategy_UseSecondAttack(const bool bIsInAir, TScriptInterface<IWeaponInterface> CurrentWeapon){}
	virtual void Strategy_UseAbility(AActor* Actor){}
	virtual void Strategy_Dodge(AActor* Actor){}
	
	void SetCurrentValues(
		UExtraMovementComponent* ExtraMoveComp,
		UCharacterMovementComponent* MoveComp,
		UTargetingComponent* TargetingComp
		) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyProperties StrategyProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStrategyMontages StrategyMontages;
};