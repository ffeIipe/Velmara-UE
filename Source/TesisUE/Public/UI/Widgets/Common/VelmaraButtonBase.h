#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "VelmaraButtonBase.generated.h"

class USizeBox;
class UOverlay;
class UBorder;
class UImage;
class UHorizontalBox;
class UCommonTextBlock;

UENUM(BlueprintType)
enum class EVelmaraButtonVariant : uint8
{
	MinimalList,
	FramedRow,
	SidebarTab,
	HexAction
};

UCLASS()
class TESISUE_API UVelmaraButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UVelmaraButtonBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetButtonText(const FText& InText);

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	FText GetButtonText() const { return ButtonLabel; }

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetVariant(EVelmaraButtonVariant InVariant);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetFontSize(int32 InFontSize);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetButtonMinSize(float InMinWidth, float InMinHeight);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetCenterAligned(bool bInCenterAligned);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetHighlightedState(bool bInHighlighted);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnVelmaraButtonHighlighted, UVelmaraButtonBase*);
	FOnVelmaraButtonHighlighted OnButtonHighlighted;

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnEnabled() override;
	virtual void NativeOnDisabled() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	void BuildButtonTree();
	void UpdateVisualState();
	bool IsVisuallyFocused() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	EVelmaraButtonVariant Variant = EVelmaraButtonVariant::MinimalList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	FText ButtonLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	int32 LabelFontSize = 21;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	float ButtonMinWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	float ButtonMinHeight = 52.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velmara|UI")
	bool bCenterAligned = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SolidBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BgGradient;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TopHairline;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BottomHairline;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ContentRow;

	UPROPERTY(Transient)
	TObjectPtr<UImage> LeftMarker;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> LabelText;

	bool bIsButtonPressed = false;
	bool bIsInFocusPath = false;
	bool bExternalHighlight = false;
};
