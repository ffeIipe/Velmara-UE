#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonInputTypeEnum.h"
#include "VelmaraFooterBar.generated.h"

class UHorizontalBox;
class UCommonActionWidget;
class UCommonTextBlock;
class UButton;

USTRUCT(BlueprintType)
struct TESISUE_API FVelmaraFooterAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHighlighted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	FVelmaraFooterAction() = default;

	FVelmaraFooterAction(FName InRowName, const FText& InLabel, bool bInHighlighted = false, bool bInEnabled = true)
		: ActionRowName(InRowName)
		, Label(InLabel)
		, bHighlighted(bInHighlighted)
		, bEnabled(bInEnabled)
	{
	}
};

UCLASS()
class TESISUE_API UVelmaraFooterBar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVelmaraFooterBar(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetActions(const TArray<FVelmaraFooterAction>& InActions);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetShowMotto(bool bShow);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFooterActionClicked, FName);
	FOnFooterActionClicked OnFooterActionClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildFooterTree();
	void RebuildActionPrompts();
	void HandleInputMethodChanged(ECommonInputType NewInputType);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> MottoBox;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ActionsBox;

	TArray<FVelmaraFooterAction> CurrentActions;
	bool bShowMotto = true;
	FDelegateHandle InputMethodChangedHandle;
};
