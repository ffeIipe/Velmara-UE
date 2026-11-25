#pragma once

#include "CoreMinimal.h"
#include "StageZeroLoadingWidget.h"
#include "VelmaraLoadingScreenWidget.generated.h"

class UImage;
class UCommonTextBlock;

UCLASS()
class TESISUE_API UVelmaraLoadingScreenWidget : public UStageZeroLoadingWidget
{
	GENERATED_BODY()

public:
	UVelmaraLoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void SelectRandomTip();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildLoadingTree();

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> TipBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpinnerImage;

	float SpinnerAngleDeg = 0.0f;
};
