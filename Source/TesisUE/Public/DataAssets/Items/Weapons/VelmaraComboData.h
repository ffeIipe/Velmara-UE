#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VelmaraComboData.generated.h"

USTRUCT(BlueprintType)
struct FComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName SectionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NextSectionOnTap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NextSectionOnHold;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamageMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class TESISUE_API UVelmaraComboData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TMap<FName, FComboStep> ComboNodes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	FName StartSectionName = "Combo01";
};