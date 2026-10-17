#pragma once

#include "CoreMinimal.h"
#include "Features/AttributeTreeSystem/Data/AttributeTreeTypes.h"
#include "Features/UpgradeSystem/UI/UpgradeMenuWidgetBase.h"
#include "AttributeTreeMenuWidget.generated.h"

class UAttributeTreeComponent;
class UAttributeTreeRowWidget;
class UTextBlock;
class UVerticalBox;

/*
 * Menú del árbol de atributos. Construye su layout por código, así que funciona sin un Widget Blueprint.
 * Lo abre/cierra UAttributeTreeComponent::ToggleMenu(). El cierre por teclado lo maneja UUpgradeMenuWidgetBase.
 */
UCLASS()
class TESISUE_API UAttributeTreeMenuWidget : public UUpgradeMenuWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Attribute Tree")
	UAttributeTreeComponent* GetAttributeTree() const;

	virtual void RefreshMenu() override;

	// Lo llaman las filas tras intentar una compra.
	void ShowUpgradeResult(EAttributeUpgradeResult Result, const FGameplayTag& UpgradeTag);

protected:
	virtual void NativeOnInitialized() override;

	virtual void BindToUpgradeComponent() override;

	virtual void UnbindFromUpgradeComponent() override;

	virtual void OnUpgradeComponentSet() override;

	UPROPERTY(EditDefaultsOnly, Category = "Attribute Tree")
	TSubclassOf<UAttributeTreeRowWidget> RowWidgetClass;

private:
	void BuildLayout();

	void RebuildRows();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleAttributeUpgraded(FGameplayTag UpgradeTag, int32 NewLevel);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CurrencyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAttributeTreeRowWidget>> Rows;
};
