#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "AttributeTreeRowWidget.generated.h"

class UAttributeTreeComponent;
class UAttributeTreeMenuWidget;
class UButton;
class UHorizontalBox;
class UImage;
class USizeBox;
class UTextBlock;

/*
 * Fila del menú: muestra una rama del árbol (nombre, niveles I..V conectados, bonus actual/siguiente) y el botón de mejora.
 * Construye su layout por código.
 */
UCLASS()
class TESISUE_API UAttributeTreeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UAttributeTreeComponent* InAttributeTree, const FGameplayTag& InUpgradeTag, UAttributeTreeMenuWidget* InOwnerMenu);

	void Refresh();

	FGameplayTag GetUpgradeTag() const { return UpgradeTag; }

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildLayout();

	void RebuildLevelPips(int32 CurrentLevel, int32 MaxLevel, bool bCanAffordNext);

	UFUNCTION()
	void HandleUpgradeClicked();

	TWeakObjectPtr<UAttributeTreeComponent> AttributeTree;

	TWeakObjectPtr<UAttributeTreeMenuWidget> OwnerMenu;

	FGameplayTag UpgradeTag;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> IconBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> PipsBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> UpgradeButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ButtonLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ButtonCostText;
};
