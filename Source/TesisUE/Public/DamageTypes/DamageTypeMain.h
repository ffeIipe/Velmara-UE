// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DamageTypeMain.generated.h"

UENUM(BlueprintType)
enum class EMainDamageTypes : uint8
{
	EMDT_CrashDown UMETA(DisplayName = "CrashDown")
};

UCLASS()
class TESISUE_API UDamageTypeMain : public UDamageType
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMainDamageTypes DamageType;
};
