// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MontagesData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FMontages
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	UAnimMontage* LaunchMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	UAnimMontage* DeathMontage;
};

UCLASS()
class TESISUE_API UMontagesData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	FMontages Montages;
};