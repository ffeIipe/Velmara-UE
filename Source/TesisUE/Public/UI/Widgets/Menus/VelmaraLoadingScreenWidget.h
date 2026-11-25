#pragma once

#include "CoreMinimal.h"
#include "StageZeroLoadingWidget.h"
#include "VelmaraLoadingScreenWidget.generated.h"

class UImage;
class UCommonTextBlock;
class IInputProcessor;

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
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	bool bFreezesGame = true;

private:
	void BuildLoadingTree();
	void BlockInput();
	void UnblockInput();
	void PauseGameOnceSettled(float DeltaTime);

	TSharedPtr<IInputProcessor> InputBlocker;
	float SettleElapsedSeconds = 0.0f;
	bool bPausedGame = false;

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> TipBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SpinnerImage;

	float SpinnerAngleDeg = 0.0f;
};

UCLASS()
class TESISUE_API UVelmaraBootLoadingScreenWidget : public UVelmaraLoadingScreenWidget
{
	GENERATED_BODY()

public:
	UVelmaraBootLoadingScreenWidget(const FObjectInitializer& ObjectInitializer);
};
