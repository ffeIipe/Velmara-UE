#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DramaticBar.generated.h"

class UProgressBar;

UCLASS()
class TESISUE_API UDramaticBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DramaticBar")
	void SetPercent(float NewPercent, bool bInstant = false);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MainBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> GhostBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float FillSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float GhostDelay = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float GhostSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Flash")
	FLinearColor FlashColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Flash")
	float FlashDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Shake")
	float ShakeIntensity = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Shake")
	float ShakeDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Pulse")
	float LowThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Pulse")
	float PulseSpeed = 6.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float Target = 1.f;
	float Display = 1.f;
	float Ghost = 1.f;
	float GhostHold = 0.f;
	float FlashLeft = 0.f;
	float ShakeLeft = 0.f;
	float PulseTime = 0.f;
	bool bInitialized = false;
	FLinearColor BaseColor = FLinearColor::White;
};
