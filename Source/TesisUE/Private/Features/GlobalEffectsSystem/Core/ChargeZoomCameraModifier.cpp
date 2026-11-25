#include "Features/GlobalEffectsSystem/Core/ChargeZoomCameraModifier.h"

#include "Curves/CurveFloat.h"

void UChargeZoomCameraModifier::StartZoom(const FCameraZoomType& Config, const float InZoomInDuration, UObject* InSource)
{
	ZoomInCurve = Config.Curve;
	ZoomInDuration = FMath::Max(InZoomInDuration, KINDA_SMALL_NUMBER);
	ReleaseDuration = FMath::Max(Config.ReleaseDuration, KINDA_SMALL_NUMBER);
	TargetOffset = Config.FOVOffset;
	StartOffset = CurrentOffset;
	ElapsedTime = 0.0f;
	Source = InSource;
	bHeldBySource = InSource != nullptr;
	Phase = EPhase::ZoomingIn;
}

void UChargeZoomCameraModifier::ReleaseZoom(const UObject* InSource)
{
	if (Phase == EPhase::ZoomingIn && bHeldBySource && Source.Get() == InSource)
	{
		BeginRelease();
	}
}

void UChargeZoomCameraModifier::BeginRelease()
{
	StartOffset = CurrentOffset;
	ElapsedTime = 0.0f;
	Phase = EPhase::Releasing;
}

bool UChargeZoomCameraModifier::ModifyCamera(const float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	if (Phase == EPhase::Idle) return false;

	ElapsedTime += DeltaTime;

	if (Phase == EPhase::ZoomingIn)
	{
		const float Progress = FMath::Clamp(ElapsedTime / ZoomInDuration, 0.0f, 1.0f);
		const float ShapedAlpha = ZoomInCurve ? ZoomInCurve->GetFloatValue(Progress) : FMath::InterpEaseIn(0.0f, 1.0f, Progress, 2.0f);
		CurrentOffset = FMath::Lerp(StartOffset, TargetOffset, ShapedAlpha);

		const bool bSourceLost = bHeldBySource && !Source.IsValid();
		const bool bOneShotFinished = !bHeldBySource && Progress >= 1.0f;
		if (bSourceLost || bOneShotFinished)
		{
			BeginRelease();
		}
	}
	else
	{
		const float Progress = FMath::Clamp(ElapsedTime / ReleaseDuration, 0.0f, 1.0f);
		CurrentOffset = FMath::InterpEaseOut(StartOffset, 0.0f, Progress, 2.0f);

		if (Progress >= 1.0f)
		{
			Phase = EPhase::Idle;
		}
	}

	InOutPOV.FOV -= CurrentOffset;
	return false;
}
