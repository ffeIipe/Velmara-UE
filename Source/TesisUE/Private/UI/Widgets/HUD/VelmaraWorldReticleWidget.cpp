#include "UI/Widgets/HUD/VelmaraWorldReticleWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "UI/Style/VelmaraUITheme.h"

TSharedRef<SWidget> UVelmaraLockOnReticleWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LockOnReticleRoot"));
		RootOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootOverlay;

		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LockOnReticleSize"));
		RootSize->SetWidthOverride(64.0f);
		RootSize->SetHeightOverride(64.0f);
		RootSize->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UOverlaySlot* RootSlot = RootOverlay->AddChildToOverlay(RootSize))
		{
			RootSlot->SetHorizontalAlignment(HAlign_Center);
			RootSlot->SetVerticalAlignment(VAlign_Center);
		}

		UOverlay* ReticleStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LockOnReticleStack"));
		ReticleStack->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReticleStack->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		RootSize->SetContent(ReticleStack);

		const FSlateBrush ReticleBrush = FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::ReticleLockOn),
			FVector2D(64.0f, 64.0f),
			FVelmaraUITheme::BloodBright());

		const TCHAR* LayerNames[2] = { TEXT("LockOnReticleImg"), TEXT("LockOnReticleBoost") };
		for (int32 LayerIndex = 0; LayerIndex < 2; ++LayerIndex)
		{
			UImage* Layer = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), LayerNames[LayerIndex]);
			Layer->SetVisibility(ESlateVisibility::HitTestInvisible);
			Layer->SetBrush(ReticleBrush);
			if (UOverlaySlot* LayerSlot = ReticleStack->AddChildToOverlay(Layer))
			{
				LayerSlot->SetHorizontalAlignment(HAlign_Fill);
				LayerSlot->SetVerticalAlignment(VAlign_Fill);
			}
			if (!ReticleImage)
			{
				ReticleImage = Layer;
			}
		}
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
		if (UWidget* Parent = ReticleImage->GetParent())
		{
			Parent->SetRenderScale(FVector2D(Scale, Scale));
		}
	}
}

TSharedRef<SWidget> UVelmaraPossessionReticleWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PossessionReticleRoot"));
		RootOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootOverlay;

		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PossessionReticleSize"));
		RootSize->SetWidthOverride(60.0f);
		RootSize->SetHeightOverride(60.0f);
		RootSize->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UOverlaySlot* RootSlot = RootOverlay->AddChildToOverlay(RootSize))
		{
			RootSlot->SetHorizontalAlignment(HAlign_Center);
			RootSlot->SetVerticalAlignment(VAlign_Center);
		}

		ReticleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PossessionReticleImg"));
		ReticleImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReticleImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ReticleImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::ReticlePossession),
			FVector2D(60.0f, 60.0f),
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
