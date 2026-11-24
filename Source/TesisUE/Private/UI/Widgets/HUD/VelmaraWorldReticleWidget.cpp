#include "UI/Widgets/HUD/VelmaraWorldReticleWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "UI/Style/VelmaraUITheme.h"

TSharedRef<SWidget> UVelmaraLockOnReticleWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LockOnReticleSize"));
		RootSize->SetWidthOverride(72.0f);
		RootSize->SetHeightOverride(72.0f);
		WidgetTree->RootWidget = RootSize;

		ReticleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LockOnReticleImg"));
		ReticleImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ReticleImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::ReticleLockOn),
			FVector2D(72.0f, 72.0f),
			FVelmaraUITheme::BloodBright()));
		RootSize->SetContent(ReticleImage);
	}
	return Super::RebuildWidget();
}

void UVelmaraLockOnReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ReticleImage)
	{
		PulseTime += InDeltaTime * 4.0f;
		const float Scale = 0.94f + 0.06f * FMath::Sin(PulseTime);
		ReticleImage->SetRenderScale(FVector2D(Scale, Scale));
	}
}

TSharedRef<SWidget> UVelmaraPossessionReticleWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PossessionReticleSize"));
		RootSize->SetWidthOverride(84.0f);
		RootSize->SetHeightOverride(84.0f);
		WidgetTree->RootWidget = RootSize;

		ReticleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PossessionReticleImg"));
		ReticleImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ReticleImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::ReticlePossession),
			FVector2D(84.0f, 84.0f),
			FVelmaraUITheme::Spectral()));
		RootSize->SetContent(ReticleImage);
	}
	return Super::RebuildWidget();
}

void UVelmaraPossessionReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ReticleImage)
	{
		RotationAngle = FMath::Fmod(RotationAngle + 25.0f * InDeltaTime, 360.0f);
		ReticleImage->SetRenderTransformAngle(RotationAngle);
	}
}
