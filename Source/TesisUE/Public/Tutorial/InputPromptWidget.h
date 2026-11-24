#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputTypeEnum.h"
#include "Tutorial/InputPromptData.h"
#include "InputPromptWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class TESISUE_API UInputPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Velmara|Prompts")
	void InitializePrompt(const FInputPromptData& PromptData, bool bUsingGamepad);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Prompts")
	void SetHoldProgress(float NormalizedProgress);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DisplayName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

private:
	void BuildDefaultPromptTree();
	void RefreshIconForInputType(bool bUsingGamepad);
	void HandleInputMethodChanged(ECommonInputType NewInputType);

	UPROPERTY(Transient)
	TObjectPtr<UImage> DiamondFrameImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RadialProgressImage;

	FInputPromptData CachedPromptData;
	bool bHasPromptData = false;
	FDelegateHandle InputMethodHandle;
};
