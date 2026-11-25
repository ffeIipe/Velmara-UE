#include "UI/Widgets/HUD/VelmaraWorldReticleWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "UI/Style/VelmaraUITheme.h"

static FSlateBrush MakeVectorCircleBrush(const FLinearColor& OutlineColor, float OutlineWidth)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(FLinearColor::Transparent);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Brush.OutlineSettings.Width = OutlineWidth;
	Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
	Brush.OutlineSettings.bUseBrushTransparency = false;
	return Brush;
}

TSharedRef<SWidget> UVelmaraLockOnReticleWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LockOnReticleRoot"));
		RootOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootOverlay;

		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LockOnReticleSize"));
		RootSize->SetWidthOverride(48.0f);
		RootSize->SetHeightOverride(48.0f);
		RootSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

		if (UOverlaySlot* RootSlot = RootOverlay->AddChildToOverlay(RootSize))
		{
			RootSlot->SetHorizontalAlignment(HAlign_Center);
			RootSlot->SetVerticalAlignment(VAlign_Center);
		}

		UOverlay* ReticleGroup = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LockOnReticleGroup"));
		ReticleGroup->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootSize->SetContent(ReticleGroup);

		auto AddCenteredCircle = [this, ReticleGroup](float Diameter, const FLinearColor& Color, float Width)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(Diameter);
			Box->SetHeightOverride(Diameter);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);

			UBorder* Ring = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Ring->SetPadding(FMargin(0.0f));
			Ring->SetBrush(MakeVectorCircleBrush(Color, Width));
			Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
			Box->SetContent(Ring);

			if (UOverlaySlot* Slot = ReticleGroup->AddChildToOverlay(Box))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddCenteredCircle(30.0f, FVelmaraUITheme::BloodBright(0.95f), 1.5f);

		auto AddNeedle = [this, ReticleGroup](float Width, float Height, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Pad)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(Width);
			Box->SetHeightOverride(Height);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);

			UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Bar->SetPadding(FMargin(0.0f));
			Bar->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::BloodBright(0.95f)));
			Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
			Box->SetContent(Bar);

			if (UOverlaySlot* Slot = ReticleGroup->AddChildToOverlay(Box))
			{
				Slot->SetHorizontalAlignment(HAlign);
				Slot->SetVerticalAlignment(VAlign);
				Slot->SetPadding(Pad);
			}
		};

		AddNeedle(1.5f, 14.0f, HAlign_Center, VAlign_Top, FMargin(0.0f, 1.0f, 0.0f, 0.0f));
		AddNeedle(1.5f, 14.0f, HAlign_Center, VAlign_Bottom, FMargin(0.0f, 0.0f, 0.0f, 1.0f));
		AddNeedle(14.0f, 1.5f, HAlign_Left, VAlign_Center, FMargin(1.0f, 0.0f, 0.0f, 0.0f));
		AddNeedle(14.0f, 1.5f, HAlign_Right, VAlign_Center, FMargin(0.0f, 0.0f, 1.0f, 0.0f));

		USizeBox* CenterBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CenterBox->SetWidthOverride(6.0f);
		CenterBox->SetHeightOverride(6.0f);
		CenterBox->SetVisibility(ESlateVisibility::HitTestInvisible);

		ReticleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LockOnReticleImg"));
		ReticleImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReticleImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
			FVector2D(6.0f, 6.0f),
			FVelmaraUITheme::Bone(0.95f)));
		CenterBox->SetContent(ReticleImage);

		if (UOverlaySlot* CenterSlot = ReticleGroup->AddChildToOverlay(CenterBox))
		{
			CenterSlot->SetHorizontalAlignment(HAlign_Center);
			CenterSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UVelmaraLockOnReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UOverlay* RootOverlay = WidgetTree ? Cast<UOverlay>(WidgetTree->RootWidget) : nullptr;
	UWidget* AnimTarget = RootOverlay ? RootOverlay->GetChildAt(0) : nullptr;
	if (AnimTarget)
	{
		PulseTime += InDeltaTime * 4.0f;
		const float Scale = 0.94f + 0.06f * FMath::Sin(PulseTime);
		AnimTarget->SetRenderScale(FVector2D(Scale, Scale));
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
		RootSize->SetWidthOverride(54.0f);
		RootSize->SetHeightOverride(54.0f);
		RootSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootSize->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

		if (UOverlaySlot* RootSlot = RootOverlay->AddChildToOverlay(RootSize))
		{
			RootSlot->SetHorizontalAlignment(HAlign_Center);
			RootSlot->SetVerticalAlignment(VAlign_Center);
		}

		UOverlay* ReticleGroup = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PossessionReticleGroup"));
		ReticleGroup->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootSize->SetContent(ReticleGroup);

		auto AddCenteredCircle = [this, ReticleGroup](float Diameter, const FLinearColor& Color, float Width)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(Diameter);
			Box->SetHeightOverride(Diameter);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);

			UBorder* Ring = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Ring->SetPadding(FMargin(0.0f));
			Ring->SetBrush(MakeVectorCircleBrush(Color, Width));
			Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
			Box->SetContent(Ring);

			if (UOverlaySlot* Slot = ReticleGroup->AddChildToOverlay(Box))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddCenteredCircle(44.0f, FVelmaraUITheme::Spectral(0.95f), 1.5f);
		AddCenteredCircle(34.0f, FVelmaraUITheme::Spectral(0.55f), 1.0f);

		auto AddCardinalDiamond = [this, ReticleGroup](EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Pad)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(6.0f);
			Box->SetHeightOverride(6.0f);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);

			UImage* Pip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Pip->SetVisibility(ESlateVisibility::HitTestInvisible);
			Pip->SetBrush(FVelmaraUITheme::MakeImageBrush(
				FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
				FVector2D(6.0f, 6.0f),
				FVelmaraUITheme::Bone(0.92f)));
			Box->SetContent(Pip);

			if (UOverlaySlot* Slot = ReticleGroup->AddChildToOverlay(Box))
			{
				Slot->SetHorizontalAlignment(HAlign);
				Slot->SetVerticalAlignment(VAlign);
				Slot->SetPadding(Pad);
			}
		};

		AddCardinalDiamond(HAlign_Center, VAlign_Top, FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		AddCardinalDiamond(HAlign_Center, VAlign_Bottom, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
		AddCardinalDiamond(HAlign_Left, VAlign_Center, FMargin(2.0f, 0.0f, 0.0f, 0.0f));
		AddCardinalDiamond(HAlign_Right, VAlign_Center, FMargin(0.0f, 0.0f, 2.0f, 0.0f));

		USizeBox* CenterBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CenterBox->SetWidthOverride(5.0f);
		CenterBox->SetHeightOverride(5.0f);
		CenterBox->SetVisibility(ESlateVisibility::HitTestInvisible);

		ReticleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PossessionReticleImg"));
		ReticleImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		ReticleImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondHollow),
			FVector2D(5.0f, 5.0f),
			FVelmaraUITheme::Spectral(0.90f)));
		CenterBox->SetContent(ReticleImage);

		if (UOverlaySlot* CenterSlot = ReticleGroup->AddChildToOverlay(CenterBox))
		{
			CenterSlot->SetHorizontalAlignment(HAlign_Center);
			CenterSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UVelmaraPossessionReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UOverlay* RootOverlay = WidgetTree ? Cast<UOverlay>(WidgetTree->RootWidget) : nullptr;
	UWidget* AnimTarget = RootOverlay ? RootOverlay->GetChildAt(0) : nullptr;
	if (AnimTarget)
	{
		RotationAngle = FMath::Fmod(RotationAngle + 25.0f * InDeltaTime, 360.0f);
		AnimTarget->SetRenderTransformAngle(RotationAngle);
	}
}
