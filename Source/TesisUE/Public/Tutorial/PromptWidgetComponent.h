#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "PromptWidgetComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UPromptWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
    UPromptWidgetComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Prompt")
    void InitializeFromDataTable(UDataTable* InDataTable, const FName& InRowName, bool bIsGamepad);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prompt")
    UDataTable* PromptDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prompt")
    FName PromptRowName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prompt")
    bool bUseGamepadIcon;

    void LoadAndApplyPrompt();
};
