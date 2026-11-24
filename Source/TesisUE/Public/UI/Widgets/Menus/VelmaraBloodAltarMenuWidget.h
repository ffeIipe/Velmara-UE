#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "VelmaraBloodAltarMenuWidget.generated.h"

class AActor;
class APlayerController;
class UAttributeTreeComponent;
class UAttributeTreeMenuWidget;
class UButton;
class UCommonTextBlock;
class UImage;
class UProgressionComponent;
class UUpgradeComponentBase;
class UVelmaraBracketPanel;
class UVelmaraFooterBar;
class UWeaponData;
class UWeaponUpgradeComponent;
class UWeaponUpgradeMenuWidget;
class UWidgetSwitcher;

UENUM(BlueprintType)
enum class EVelmaraBloodAltarTab : uint8
{
	Attributes,
	WeaponUpgrades
};

UCLASS()
class TESISUE_API UVelmaraBloodAltarMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UVelmaraBloodAltarMenuWidget(const FObjectInitializer& ObjectInitializer);

	static UVelmaraBloodAltarMenuWidget* OpenBloodAltar(APlayerController* PlayerController, AActor* TargetActor, EVelmaraBloodAltarTab InitialTab = EVelmaraBloodAltarTab::Attributes);
	static UVelmaraBloodAltarMenuWidget* GetActiveAltar(const UObject* WorldContext);
	static bool IsActiveForComponent(const UUpgradeComponentBase* Component);
	static void CloseActiveAltar(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void BindTargetActor(AActor* TargetActor, EVelmaraBloodAltarTab InitialTab = EVelmaraBloodAltarTab::Attributes);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SetActiveTab(EVelmaraBloodAltarTab NewTab);

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	EVelmaraBloodAltarTab GetActiveTab() const { return ActiveTab; }

	bool OwnsComponent(const UUpgradeComponentBase* Component) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

private:
	void BuildScreenTree();
	void RefreshCurrencyHeader();
	void RefreshTabVisuals();
	void UpdateDetailsForAttribute(FGameplayTag UpgradeTag);
	void UpdateDetailsForCombo(UWeaponData* Weapon, FGameplayTag UnlockTag);
	void ShowFeedback(const FText& Message, bool bIsError);
	void ApplyPauseState(bool bPaused);
	void BindCurrencyDelegates();
	void UnbindCurrencyDelegates();
	int32 ResolveBloodAmount() const;

	UFUNCTION()
	void HandleAttributesTabClicked();

	UFUNCTION()
	void HandleWeaponsTabClicked();

	UFUNCTION()
	void HandleCurrencyChanged(int32 NewAmount);

	void HandleAttributeRowSelected(FGameplayTag UpgradeTag);
	void HandleWeaponComboSelected(UWeaponData* Weapon, FGameplayTag UnlockTag);
	void HandleChildFeedback(const FText& Message, bool bIsError);

	TWeakObjectPtr<UAttributeTreeComponent> AttributeTreeComp;
	TWeakObjectPtr<UWeaponUpgradeComponent> WeaponUpgradeComp;
	TWeakObjectPtr<UProgressionComponent> ProgressionComp;

	EVelmaraBloodAltarTab ActiveTab = EVelmaraBloodAltarTab::Attributes;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AttributesTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> AttributesTabText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> AttributesTabLine;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WeaponsTabButton;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> WeaponsTabText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> WeaponsTabLine;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> CurrencyValueText;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UAttributeTreeMenuWidget> AttributeTreeWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWeaponUpgradeMenuWidget> WeaponUpgradeWidget;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraBracketPanel> DetailsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailCategoryText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailMetaText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailCostText;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> DetailFeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UVelmaraFooterBar> FooterBar;
};
