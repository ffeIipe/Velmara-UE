#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Features/GlobalEffectsSystem/Data/Types.h"
#include "ChargeZoomCameraModifier.generated.h"

UCLASS()
class TESISUE_API UChargeZoomCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	void StartZoom(const FCameraZoomType& Config, float InZoomInDuration, UObject* InSource);

	void ReleaseZoom(const UObject* InSource);

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	enum class EPhase : uint8 { Idle, ZoomingIn, Releasing };

	void BeginRelease();

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> ZoomInCurve;

	TWeakObjectPtr<UObject> Source;

	EPhase Phase = EPhase::Idle;
	bool bHeldBySource = false;
	float ElapsedTime = 0.0f;
	float ZoomInDuration = 0.0f;
	float ReleaseDuration = 0.0f;
	float TargetOffset = 0.0f;
	float StartOffset = 0.0f;
	float CurrentOffset = 0.0f;
};
