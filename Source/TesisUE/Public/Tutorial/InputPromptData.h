#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "InputPromptData.generated.h"

USTRUCT(BlueprintType)
struct FInputPromptData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName InputName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UTexture2D* KeyboardIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UTexture2D* GamepadIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayText;
};
