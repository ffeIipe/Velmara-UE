#include "HUD/DramaticBar.h"
#include "Components/ProgressBar.h"

void UDramaticBar::NativeConstruct()
{
	Super::NativeConstruct();
	if (MainBar)
	{
		BaseColor = MainBar->GetFillColorAndOpacity();
	}
}

void UDramaticBar::SetPercent(float NewPercent, bool bInstant)
{
	NewPercent = FMath::Clamp(NewPercent, 0.f, 1.f);

	if (!bInitialized || bInstant)
	{
		bInitialized = true;
		Target = Display = Ghost = NewPercent;
	}
	else if (NewPercent < Target)
	{
		Display = NewPercent; // la caida del fill es instantanea; la estela blanca la persigue
		GhostHold = GhostDelay;
		FlashLeft = FlashDuration;
		ShakeLeft = ShakeDuration;
	}
	else if (NewPercent > Target)
	{
		Ghost = NewPercent; // al subir, la fantasma marca el destino y el fill la persigue
	}
	Target = NewPercent;
}

void UDramaticBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!MainBar)
	{
		return;
	}

	Display = FMath::FInterpTo(Display, Target, InDeltaTime, FillSpeed);
	MainBar->SetPercent(Display);

	if (GhostBar)
	{
		if (Ghost > Target)
		{
			GhostHold -= InDeltaTime;
			if (GhostHold <= 0.f)
			{
				Ghost = FMath::FInterpTo(Ghost, Target, InDeltaTime, GhostSpeed);
			}
		}
		GhostBar->SetPercent(Ghost);
	}

	// Flash: del color de flash vuelve al color base
	FlashLeft = FMath::Max(0.f, FlashLeft - InDeltaTime);
	const float FlashAlpha = FlashDuration > 0.f ? FlashLeft / FlashDuration : 0.f;
	MainBar->SetFillColorAndOpacity(FMath::Lerp(BaseColor, FlashColor, FlashAlpha));

	// Shake: oscila y se amortigua
	ShakeLeft = FMath::Max(0.f, ShakeLeft - InDeltaTime);
	const float ShakeAlpha = ShakeDuration > 0.f ? ShakeLeft / ShakeDuration : 0.f;
	SetRenderTranslation(FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * ShakeIntensity * ShakeAlpha);

	// Pulso con valor bajo
	if (LowThreshold > 0.f && Target > 0.f && Target <= LowThreshold)
	{
		PulseTime += InDeltaTime * PulseSpeed;
		MainBar->SetRenderOpacity(0.6f + 0.4f * (0.5f + 0.5f * FMath::Sin(PulseTime)));
	}
	else
	{
		MainBar->SetRenderOpacity(1.f);
	}
}
