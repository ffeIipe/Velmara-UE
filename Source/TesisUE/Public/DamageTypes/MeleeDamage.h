// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "MeleeDamage.generated.h"

UENUM(BlueprintType)
enum class EMeleeDamageTypes : uint8
{
	EMDT_None		UMETA(DisplayName = "None"),
	EMDT_CrashDown	UMETA(DisplayName = "CrashDown"),
	EMDT_InAir		UMETA(DisplayName = "InAir"),
	EMDT_Finisher	UMETA(DisplayName = "Finisher"),
	EMDT_Puncture	UMETA(DisplayName = "Puncture"),
	EMDT_Impact		UMETA(DisplayName = "Impact"),
	EMDT_Slash		UMETA(DisplayName = "Slash"),
	EMDT_Launch		UMETA(DisplayName = "Launch")
};

UCLASS()
class TESISUE_API UMeleeDamage : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMeleeDamageTypes DamageType;
};