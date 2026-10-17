#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

/*
 * Estilo compartido de los menús de mejora (colores, fuentes y brushes).
 * Se usan funciones en lugar de constantes globales para evitar problemas de orden de inicialización estática.
 */
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
		inline FLinearColor Overlay()       { return Hex(TEXT("05040A"), 0.72f); }
		inline FLinearColor Panel()         { return Hex(TEXT("121017"), 0.98f); }
		inline FLinearColor PanelOutline()  { return Hex(TEXT("5C4A26")); }
		inline FLinearColor Row()           { return Hex(TEXT("1C1922")); }
		inline FLinearColor RowOutline()    { return Hex(TEXT("2C2833")); }
		inline FLinearColor Gold()          { return Hex(TEXT("D9A441")); }
		inline FLinearColor GoldDim()       { return Hex(TEXT("7A6230")); }
		inline FLinearColor TextPrimary()   { return Hex(TEXT("EDE6D6")); }
		inline FLinearColor TextSecondary() { return Hex(TEXT("9C94A3")); }
		inline FLinearColor TextOnGold()    { return Hex(TEXT("1A1408")); }
		inline FLinearColor Locked()        { return Hex(TEXT("2A2630")); }
		inline FLinearColor LockedText()    { return Hex(TEXT("6B6472")); }
		inline FLinearColor Success()       { return Hex(TEXT("7FC97F")); }
		inline FLinearColor Error()         { return Hex(TEXT("E0605A")); }

		inline FLinearColor ButtonNormal()  { return Hex(TEXT("8C6A23")); }
		inline FLinearColor ButtonHovered() { return Hex(TEXT("A8822F")); }
		inline FLinearColor ButtonPressed() { return Hex(TEXT("6E5219")); }
		inline FLinearColor RowHovered()    { return Hex(TEXT("27232E")); }

		// Paleta del altar de sangre
		inline FLinearColor Blood()         { return Hex(TEXT("C4293F")); }
		inline FLinearColor BloodDim()      { return Hex(TEXT("6E1A27")); }
		inline FLinearColor BloodPanel()    { return Hex(TEXT("140D10"), 0.98f); }
		inline FLinearColor BloodRow()      { return Hex(TEXT("221418")); }
		inline FLinearColor BloodHovered()  { return Hex(TEXT("D93A51")); }
		inline FLinearColor BloodPressed()  { return Hex(TEXT("8E1D2D")); }
		inline FLinearColor TextOnBlood()   { return Hex(TEXT("FBEDEF")); }
	}

	inline FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, const float Radius,
	                                    const FLinearColor& Outline = FLinearColor::Transparent, const float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	inline FButtonStyle MakeButtonStyle(const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed,
	                                    const FLinearColor& Outline = FLinearColor::Transparent, const float OutlineWidth = 0.f)
	{
		FButtonStyle Style;
		Style.SetNormal(MakeRoundedBrush(Normal, 8.f, Outline, OutlineWidth));
		Style.SetHovered(MakeRoundedBrush(Hovered, 8.f, Outline, OutlineWidth));
		Style.SetPressed(MakeRoundedBrush(Pressed, 8.f, Outline, OutlineWidth));
		Style.SetDisabled(MakeRoundedBrush(Colors::Locked(), 8.f));
		Style.SetNormalPadding(FMargin(12.f, 6.f));
		Style.SetPressedPadding(FMargin(12.f, 7.f, 12.f, 5.f));
		return Style;
	}

	template <typename TWidget>
	TWidget* Make(UWidgetTree* WidgetTree)
	{
		return WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass());
	}

	inline void StyleText(UTextBlock* Text, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
	}

	inline UTextBlock* MakeText(UWidgetTree* WidgetTree, const FText& Content, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		UTextBlock* Text = Make<UTextBlock>(WidgetTree);
		Text->SetText(Content);
		StyleText(Text, Size, Color, bBold);
		return Text;
	}
}
