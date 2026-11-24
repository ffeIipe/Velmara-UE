#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "VelmaraConfirmModalWidget.generated.h"

class UVelmaraBracketPanel;
class UVelmaraButtonBase;
class UVelmaraFooterBar;
class UCommonTextBlock;

UCLASS()
class TESISUE_API UVelmaraConfirmModalWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraConfirmModalWidget(const FObjectInitializer& ObjectInitializer);

	static UVelmaraConfirmModalWidget* ShowConfirmModal(
		UObject* WorldContext,
		const FText& Title,
		const FText& Message,
		const FText& ConfirmLabel,
		const FText& CancelLabel,
		TFunction<void()> OnConfirm,
		TFunction<void()> OnCancel = nullptr);

	void ConfigureModal(
		const FText& Title,
		const FText& Message,
		const FText& ConfirmLabel,
		const FText& CancelLabel,
		TFunction<void()> InOnConfirm,
		TFunction<void()> InOnCancel);

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildModalTree();
	void HandleConfirmClicked();
	void HandleCancelClicked();
	void CloseModal(bool bConfirmed);

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DialogPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> CancelButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraButtonBase> ConfirmButton;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;

	FText PendingTitle;
	FText PendingMessage;
	FText PendingConfirmLabel;
	FText PendingCancelLabel;
	TFunction<void()> ConfirmCallback;
	TFunction<void()> CancelCallback;
	bool bClosed = false;
	bool bAddedDirectlyToViewport = false;
};
