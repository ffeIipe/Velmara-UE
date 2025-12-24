#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Test_VelmaraComboData.generated.h"

USTRUCT(BlueprintType)
struct FNewComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName NextNodeNameOnTap;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* NextAnimOnTap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName NextNodeNameOnHold;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* NextAnimOnHold;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DamageMultiplier = 1.0f;
};

UCLASS(BlueprintType)
class TESISUE_API UTest_VelmaraComboData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TMap<FName, FNewComboStep> ComboNodes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	UAnimMontage* StartingAnim;
};
