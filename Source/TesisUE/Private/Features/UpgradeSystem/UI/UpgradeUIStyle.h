#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UI/Style/VelmaraUITheme.h"

namespace UpgradeUIStyle
{
	inline FLinearColor Hex(const TCHAR* HexColor, const float Alpha = 1.f)
	{
		FLinearColor Color(FColor::FromHex(HexColor));
		Color.A = Alpha;
		return Color;
	}

	namespace Colors
	{
		inline FLinearColor Overlay()       { return FVelmaraUITheme::Void(0.78f); }
		inline FLinearColor Panel()         { return FVelmaraUITheme::Void(0.88f); }
		inline FLinearColor PanelOutline()  { return FVelmaraUITheme::Ash(); }
		inline FLinearColor Row()           { return FVelmaraUITheme::Charcoal(0.74f); }
		inline FLinearColor RowOutline()    { return FVelmaraUITheme::Ash(); }
		inline FLinearColor Gold()          { return FVelmaraUITheme::BloodBright(); }
		inline FLinearColor GoldDim()       { return FVelmaraUITheme::BloodDried(); }
		inline FLinearColor TextPrimary()   { return FVelmaraUITheme::Bone(); }
		inline FLinearColor TextSecondary() { return FVelmaraUITheme::BoneMuted(); }
		inline FLinearColor TextOnGold()    { return FVelmaraUITheme::Bone(); }
		inline FLinearColor Locked()        { return FVelmaraUITheme::Void(0.82f); }
		inline FLinearColor LockedText()    { return FVelmaraUITheme::BoneMuted(0.45f); }
		inline FLinearColor Success()       { return FVelmaraUITheme::Bone(); }
		inline FLinearColor Error()         { return FVelmaraUITheme::BloodBright(); }

		inline FLinearColor ButtonNormal()  { return FVelmaraUITheme::Blood(0.75f); }
		inline FLinearColor ButtonHovered() { return FVelmaraUITheme::BloodBright(); }
		inline FLinearColor ButtonPressed() { return FVelmaraUITheme::BloodDried(); }
		inline FLinearColor RowHovered()    { return FVelmaraUITheme::Blood(0.32f); }

		inline FLinearColor Blood()         { return FVelmaraUITheme::Blood(); }
		inline FLinearColor BloodDim()      { return FVelmaraUITheme::BloodDried(); }
		inline FLinearColor BloodPanel()    { return FVelmaraUITheme::Void(0.88f); }
		inline FLinearColor BloodRow()      { return FVelmaraUITheme::Charcoal(0.76f); }
		inline FLinearColor BloodHovered()  { return FVelmaraUITheme::BloodBright(); }
		inline FLinearColor BloodPressed()  { return FVelmaraUITheme::BloodDried(); }
		inline FLinearColor TextOnBlood()   { return FVelmaraUITheme::Bone(); }
	}

	inline FSlateBrush MakeSharpBrush(const FLinearColor& Fill,
	                                  const FLinearColor& Outline = FLinearColor::Transparent, const float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		if (OutlineWidth > 0.f && Outline.A > 0.f)
		{
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = FSlateColor(Fill);
			Brush.OutlineSettings = FSlateBrushOutlineSettings(0.f, FSlateColor(Outline), OutlineWidth);
		}
		else
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = FSlateColor(Fill);
		}
		return Brush;
	}

	inline FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, const float Radius,
	                                    const FLinearColor& Outline = FLinearColor::Transparent, const float OutlineWidth = 0.f)
	{
		(void)Radius;
		return MakeSharpBrush(Fill, Outline, OutlineWidth);
	}

	inline FSlateBrush MakeTransparentBrush()
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		return Brush;
	}

	inline FButtonStyle MakeTransparentButtonStyle()
	{
		FButtonStyle Style;
		const FSlateBrush Empty = MakeTransparentBrush();
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		FVelmaraUITheme::ApplyButtonSounds(Style);
		return Style;
	}

	inline FButtonStyle MakeButtonStyle(const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed,
	                                    const FLinearColor& Outline = FLinearColor::Transparent, const float OutlineWidth = 0.f)
	{
		FButtonStyle Style;
		Style.SetNormal(MakeSharpBrush(Normal, Outline, OutlineWidth));
		Style.SetHovered(MakeSharpBrush(Hovered, FVelmaraUITheme::BloodBright(), FMath::Max(OutlineWidth, 1.f)));
		Style.SetPressed(MakeSharpBrush(Pressed, FVelmaraUITheme::BloodBright(), FMath::Max(OutlineWidth, 1.f)));
		Style.SetDisabled(MakeSharpBrush(Colors::Locked(), FVelmaraUITheme::Ash(0.5f), 1.f));
		Style.SetNormalPadding(FMargin(14.f, 6.f));
		Style.SetPressedPadding(FMargin(14.f, 7.f, 14.f, 5.f));
		FVelmaraUITheme::ApplyButtonSounds(Style);
		return Style;
	}

	template <typename TWidget>
	TWidget* Make(UWidgetTree* WidgetTree)
	{
		return WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass());
	}

	inline void StyleText(UTextBlock* Text, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		const FSlateFontInfo Font = bBold
			? FVelmaraUITheme::GetBoldBodyFont(Size, 0)
			: FVelmaraUITheme::GetBodyFont(Size, 0);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
	}

	inline void StyleDisplayText(UTextBlock* Text, const int32 Size, const FLinearColor& Color, const int32 LetterSpacing = 140)
	{
		Text->SetFont(FVelmaraUITheme::GetDisplayFont(Size, LetterSpacing));
		Text->SetColorAndOpacity(FSlateColor(Color));
	}

	inline UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& Content, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		UTextBlock* Text = Make<UTextBlock>(WidgetTree);
		Text->SetText(Content);
		StyleText(Text, Size, Color, bBold);
		return Text;
	}

	inline UTextBlock* MakeDisplayText(UWidgetTree* WidgetTree, const FText& Content, const int32 Size, const FLinearColor& Color, const int32 LetterSpacing = 140)
	{
		UTextBlock* Text = Make<UTextBlock>(WidgetTree);
		Text->SetText(Content);
		StyleDisplayText(Text, Size, Color, LetterSpacing);
		return Text;
	}
}
