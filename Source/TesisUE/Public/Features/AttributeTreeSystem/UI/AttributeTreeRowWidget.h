#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "AttributeTreeRowWidget.generated.h"

class UAttributeTreeComponent;
class UAttributeTreeMenuWidget;
class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;

UCLASS()
class TESISUE_API UAttributeTreeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UAttributeTreeComponent* InAttributeTree, const FGameplayTag& InUpgradeTag, UAttributeTreeMenuWidget* InOwnerMenu);

	void Refresh();

	FGameplayTag GetUpgradeTag() const { return UpgradeTag; }

	UWidget* GetFocusTarget() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
	void BuildLayout();

	void RebuildLevelPips(int32 CurrentLevel, int32 MaxLevel, bool bCanAffordNext);

	void RefreshVisualState();

	UTexture2D* ResolveFallbackIcon() const;

	UFUNCTION()
	void HandleUpgradeClicked();

	UFUNCTION()
	void HandleRowHovered();

	UFUNCTION()
	void HandleRowUnhovered();

	TWeakObjectPtr<UAttributeTreeComponent> AttributeTree;

	TWeakObjectPtr<UAttributeTreeMenuWidget> OwnerMenu;

	FGameplayTag UpgradeTag;

	bool bIsRowHovered = false;

	bool bIsRowFocused = false;

	bool bIsRowEnabled = true;

	bool bIsMaxed = false;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RowButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HoverWashImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameBorderImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusStarImage;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> IconBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrentValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ArrowText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NextValueText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PipsBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> CostBloodIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ChevronImage;
};
