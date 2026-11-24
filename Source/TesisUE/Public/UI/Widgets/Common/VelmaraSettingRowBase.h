#pragma once

#include "CoreMinimal.h"
#include "VelmaraSettingRowBase.generated.h"

USTRUCT(BlueprintType)
struct TESISUE_API FVelmaraSettingMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Quote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRequiresApply = false;
};
