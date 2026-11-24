#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VelmaraWorldReticleWidget.generated.h"

class UImage;

UCLASS()
class TESISUE_API UVelmaraLockOnReticleWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> ReticleImage;

	float PulseTime = 0.0f;
};

UCLASS()
class TESISUE_API UVelmaraPossessionReticleWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> ReticleImage;

	float RotationAngle = 0.0f;
};
