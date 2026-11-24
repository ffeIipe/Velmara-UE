#pragma once

#include "CoreMinimal.h"
#include "HUD/PaladinBossHealthBar.h"
#include "VelmaraBossBarWidget.generated.h"

class UCommonTextBlock;
class UProgressBar;
class UImage;
class USizeBox;

UCLASS()
class TESISUE_API UVelmaraBossBarWidget : public UPaladinBossHealthBar
{
	GENERATED_BODY()

public:
	UVelmaraBossBarWidget(const FObjectInitializer& ObjectInitializer);

	virtual void ReceivePercentagesFromBoss_Implementation(float HealthHP, float ShieldHP) override;
	virtual void SetBossDisplayName(const FText& InBossName) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildBossBarTree();

	UPROPERTY(Transient)
	TObjectPtr<UCommonTextBlock> BossDisplayNameText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ShieldRowSize;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ShieldProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> DelayedHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;

	FText BossName;
	float TargetHealthPercent = 1.0f;
	float DisplayedDelayedHealth = 1.0f;
	float DelayedHoldTimer = 0.0f;
	float TargetShieldPercent = 0.0f;
	bool bBuiltCppTree = false;
};
