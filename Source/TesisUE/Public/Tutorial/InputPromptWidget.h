#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/InputPromptData.h"
#include "InputPromptWidget.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API UInputPromptWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    void InitializePrompt(const FInputPromptData& PromptData, bool bUsingGamepad);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_DisplayName;

    UPROPERTY(meta = (BindWidget))
    class UImage* Image_Icon;
};