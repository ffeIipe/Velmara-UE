#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "VelmaraBracketPanel.generated.h"

class UOverlay;
class UBorder;
class UImage;
class UVerticalBox;
class UHorizontalBox;
class UCommonTextBlock;

UCLASS()
class TESISUE_API UVelmaraBracketPanel : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVelmaraBracketPanel(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetUseMinimalBrackets(bool bMinimal);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetBackdropAlpha(float InAlpha);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetContentPadding(const FMargin& InPadding);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetShowSignature(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetShowTopStar(bool bShow);

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UVerticalBox* GetContentBox();

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void ClearContent();

	void EnsurePanelBuilt();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildPanelTree();
	void RefreshStyle();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackdropBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BracketFrame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TopStarImage;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> SignatureRow;

	bool bUseMinimalBrackets = false;
	float BackdropAlpha = 0.80f;
	FMargin ContentPadding = FMargin(32.0f);
	bool bShowSignature = true;
	bool bShowTopStar = false;
};
