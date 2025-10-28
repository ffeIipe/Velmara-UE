#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CommandDataTable.generated.h"

class UCommand;

USTRUCT(BlueprintType)
struct FCommandDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CommandName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UCommand> CommandClass;
};
