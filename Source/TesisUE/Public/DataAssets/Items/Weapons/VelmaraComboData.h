#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VelmaraComboData.generated.h"

USTRUCT(BlueprintType)
struct FComboNode
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NextNodeOnTapName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NextNodeOnHoldName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* MontageToPlay;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	//float DamageMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class TESISUE_API UVelmaraComboData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TMap<FName, FComboNode> ComboNodes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	FName StartingNode;
};