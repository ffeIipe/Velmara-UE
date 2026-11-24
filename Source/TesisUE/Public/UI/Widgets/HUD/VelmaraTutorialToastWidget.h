#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonInputTypeEnum.h"
#include "CommonUserWidget.h"
#include "VelmaraTutorialToastWidget.generated.h"

class UVelmaraBracketPanel;
class UCommonTextBlock;
class UCommonRichTextBlock;
class UProgressBar;
class UHorizontalBox;
class USizeBox;

USTRUCT(BlueprintType)
struct TESISUE_API FVelmaraTutorialPage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText RichBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DurationSeconds = 7.5f;
};

UCLASS()
class TESISUE_API UVelmaraTutorialToastWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVelmaraTutorialToastWidget(const FObjectInitializer& ObjectInitializer);

	static UVelmaraTutorialToastWidget* GetOrCreate(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Tutorial", meta = (WorldContext = "WorldContext"))
	static void ShowTutorialPages(const UObject* WorldContext, const TArray<FVelmaraTutorialPage>& Pages);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Tutorial")
	void EnqueuePages(const TArray<FVelmaraTutorialPage>& Pages);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Tutorial")
	void AdvanceOrDismiss();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildToastTree();
	void DisplayPage(int32 PageIndex);
	void RefreshPageDots();
	void RefreshDismissHint();
	FText FormatRichBodyForCurrentInput(const FText& RawBody) const;
	void HandleInputMethodChanged(ECommonInputType NewInputType);

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ToastCardWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> CardPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> HeaderBadgeText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PageDotsBox;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonRichTextBlock> RichBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> TimerBar;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DismissHintText;

	TArray<FVelmaraTutorialPage> ActivePages;
	int32 CurrentPageIndex = INDEX_NONE;
	float PageRemainingSeconds = 0.0f;
	float PageTotalSeconds = 7.5f;
	float VisibilityAnimAlpha = 0.0f;
	bool bShowingToast = false;
	FDelegateHandle InputMethodHandle;

	static TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<UVelmaraTutorialToastWidget>> ActiveToastWidgets;
};

UCLASS()
class TESISUE_API UVelmaraLegacyPromptBridgeWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraLegacyPromptBridgeWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Tutorial")
	void ForwardPagesToToast(const TArray<FVelmaraTutorialPage>& Pages);

	UFUNCTION(BlueprintCallable, Category = "Velmara|Tutorial")
	void ExtractAndForwardFromBlueprintProperties();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

private:
	bool bForwarded = false;
	int32 TicksWaited = 0;
};
