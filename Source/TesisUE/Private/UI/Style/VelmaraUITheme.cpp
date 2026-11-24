#include "UI/Style/VelmaraUITheme.h"
#include "Engine/DataTable.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace
{
	FLinearColor FromHex(uint8 R, uint8 G, uint8 B, float Alpha)
	{
		FLinearColor Linear = FLinearColor::FromSRGBColor(FColor(R, G, B, 255));
		Linear.A = Alpha;
		return Linear;
	}
}

FLinearColor FVelmaraUITheme::Void(float Alpha) { return FromHex(0x0A, 0x08, 0x09, Alpha); }
FLinearColor FVelmaraUITheme::Charcoal(float Alpha) { return FromHex(0x1C, 0x1A, 0x1C, Alpha); }
FLinearColor FVelmaraUITheme::Ash(float Alpha) { return FromHex(0x3A, 0x36, 0x38, Alpha); }
FLinearColor FVelmaraUITheme::Bone(float Alpha) { return FromHex(0xE8, 0xE0, 0xD0, Alpha); }
FLinearColor FVelmaraUITheme::BoneMuted(float Alpha) { return FromHex(0xA3, 0x9C, 0x8E, Alpha); }
FLinearColor FVelmaraUITheme::Blood(float Alpha) { return FromHex(0x8E, 0x0F, 0x14, Alpha); }
FLinearColor FVelmaraUITheme::BloodBright(float Alpha) { return FromHex(0xC2, 0x1A, 0x1F, Alpha); }
FLinearColor FVelmaraUITheme::BloodDried(float Alpha) { return FromHex(0x4A, 0x0A, 0x0D, Alpha); }
FLinearColor FVelmaraUITheme::Shield(float Alpha) { return FromHex(0xB8, 0xC0, 0xC4, Alpha); }
FLinearColor FVelmaraUITheme::Spectral(float Alpha) { return FromHex(0x8F, 0x86, 0xA8, Alpha); }
FLinearColor FVelmaraUITheme::Sunlight(float Alpha) { return FromHex(0xD9, 0xA4, 0x41, Alpha); }

FLinearColor FVelmaraUITheme::WithAlpha(const FLinearColor& Color, float Alpha)
{
	return FLinearColor(Color.R, Color.G, Color.B, Alpha);
}

UFont* FVelmaraUITheme::LoadDisplayFont()
{
	if (UFont* Font = LoadObject<UFont>(nullptr, VelmaraUIPaths::FontDisplayPrimary))
	{
		return Font;
	}
	return LoadObject<UFont>(nullptr, VelmaraUIPaths::FontDisplayFallback);
}

UFont* FVelmaraUITheme::LoadBodyFont()
{
	if (UFont* Font = LoadObject<UFont>(nullptr, VelmaraUIPaths::FontBodyPrimary))
	{
		return Font;
	}
	return LoadObject<UFont>(nullptr, VelmaraUIPaths::FontBodyFallback);
}

UFont* FVelmaraUITheme::LoadBoldBodyFont()
{
	if (UFont* Font = LoadObject<UFont>(nullptr, VelmaraUIPaths::FontBodyBold))
	{
		return Font;
	}
	return LoadBodyFont();
}

UFont* FVelmaraUITheme::LoadItalicFont()
{
	if (UFont* Font = LoadObject<UFont>(nullptr, VelmaraUIPaths::FontBodyItalic))
	{
		return Font;
	}
	if (UFont* Fallback = LoadObject<UFont>(nullptr, VelmaraUIPaths::FontBodyItalicFallback))
	{
		return Fallback;
	}
	return LoadBodyFont();
}

FSlateFontInfo FVelmaraUITheme::GetDisplayFont(int32 Size, int32 LetterSpacing)
{
	if (UFont* Font = LoadDisplayFont())
	{
		FSlateFontInfo Info(Font, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}
	FSlateFontInfo DefaultInfo = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	DefaultInfo.LetterSpacing = LetterSpacing;
	return DefaultInfo;
}

FSlateFontInfo FVelmaraUITheme::GetBodyFont(int32 Size, int32 LetterSpacing)
{
	if (UFont* Font = LoadBodyFont())
	{
		FSlateFontInfo Info(Font, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}
	FSlateFontInfo DefaultInfo = FCoreStyle::GetDefaultFontStyle("Regular", Size);
	DefaultInfo.LetterSpacing = LetterSpacing;
	return DefaultInfo;
}

FSlateFontInfo FVelmaraUITheme::GetBoldBodyFont(int32 Size, int32 LetterSpacing)
{
	if (UFont* Font = LoadBoldBodyFont())
	{
		FSlateFontInfo Info(Font, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}
	FSlateFontInfo DefaultInfo = FCoreStyle::GetDefaultFontStyle("Bold", Size);
	DefaultInfo.LetterSpacing = LetterSpacing;
	return DefaultInfo;
}

FSlateFontInfo FVelmaraUITheme::GetItalicFont(int32 Size, int32 LetterSpacing)
{
	if (UFont* Font = LoadItalicFont())
	{
		FSlateFontInfo Info(Font, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}
	FSlateFontInfo DefaultInfo = FCoreStyle::GetDefaultFontStyle("Italic", Size);
	DefaultInfo.LetterSpacing = LetterSpacing;
	return DefaultInfo;
}

int32 FVelmaraUITheme::GetSubtitleFontSize(int32 SubtitleSizeIndex)
{
	switch (SubtitleSizeIndex)
	{
	case 0:
		return 24;
	case 2:
		return 38;
	default:
		return 30;
	}
}

UTexture2D* FVelmaraUITheme::LoadTexture(const TCHAR* AssetPath)
{
	if (!AssetPath || AssetPath[0] == TEXT('\0'))
	{
		return nullptr;
	}
	return LoadObject<UTexture2D>(nullptr, AssetPath);
}

UTexture2D* FVelmaraUITheme::LoadVelmaraLogo()
{
	if (UTexture2D* LogoV2 = LoadTexture(VelmaraUIPaths::LogoVelmaraV2))
	{
		return LogoV2;
	}
	if (UTexture2D* LogoLegacy = LoadTexture(VelmaraUIPaths::LogoVelmaraLegacy))
	{
		return LogoLegacy;
	}
	return LoadTexture(VelmaraUIPaths::LogoVelmaraTitleFallback);
}

UDataTable* FVelmaraUITheme::LoadInputActionsTable()
{
	return LoadObject<UDataTable>(nullptr, VelmaraUIPaths::InputActionsTable);
}

UDataTable* FVelmaraUITheme::LoadRichTextStyleSet()
{
	return LoadObject<UDataTable>(nullptr, VelmaraUIPaths::RichTextStyleSet);
}

UDataTable* FVelmaraUITheme::LoadShopItemDataTable()
{
	return LoadObject<UDataTable>(nullptr, VelmaraUIPaths::ShopItemDataTable);
}

UDataTable* FVelmaraUITheme::LoadInputPromptsTable()
{
	return LoadObject<UDataTable>(nullptr, VelmaraUIPaths::InputPromptsTable);
}

FSlateBrush FVelmaraUITheme::MakeSolidBrush(const FLinearColor& Color)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.TintColor = FSlateColor(Color);
	return Brush;
}

FSlateBrush FVelmaraUITheme::MakeImageBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

FSlateBrush FVelmaraUITheme::MakeBorderBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint, float ScaleFactor)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Border;
	Brush.SetResourceObject(Texture);
	Brush.Margin = Margin;
	Brush.TintColor = FSlateColor(Tint);
	if (Texture)
	{
		Brush.ImageSize = FVector2D(Texture->GetSizeX() * ScaleFactor, Texture->GetSizeY() * ScaleFactor);
	}
	return Brush;
}

FSlateBrush FVelmaraUITheme::MakeBoxBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint, float ScaleFactor)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.SetResourceObject(Texture);
	Brush.Margin = Margin;
	Brush.TintColor = FSlateColor(Tint);
	if (Texture)
	{
		Brush.ImageSize = FVector2D(Texture->GetSizeX() * ScaleFactor, Texture->GetSizeY() * ScaleFactor);
	}
	return Brush;
}

FSlateBrush FVelmaraUITheme::MakeOptionRowBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::FrameOptionRow), FMargin(0.10156f, 0.125f, 0.10156f, 0.125f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeBracketsPanelBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::FrameBracketsPanel), FMargin(0.15625f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeBracketsMinimalBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::FrameBracketsMinimal), FMargin(0.16797f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeGothicPanelBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::FramePanelGothic), FMargin(0.09766f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeHexButtonBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::BtnHex3Slice), FMargin(0.14844f, 0.15625f, 0.14844f, 0.15625f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeSquareSlotNormalBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::SlotSquareNormal), FMargin(0.13281f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeSquareSlotSelectedBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::SlotSquareSelected), FMargin(0.23438f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeSquareSlotFillBrush(const FLinearColor& Tint)
{
	return MakeBoxBrush(LoadTexture(VelmaraUIPaths::SlotSquareFillMask), FMargin(0.14062f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeHealthBarFrameBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::BarHealthFrame), FMargin(0.07031f, 0.34375f, 0.07031f, 0.34375f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeShieldBarFrameBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::BarShieldFrame), FMargin(0.05469f, 0.4375f, 0.05469f, 0.4375f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeBossBarFrameBrush(const FLinearColor& Tint)
{
	return MakeBorderBrush(LoadTexture(VelmaraUIPaths::BarBossFrame), FMargin(0.07161f, 0.3625f, 0.07161f, 0.3625f), Tint, 0.5f);
}

FSlateBrush FVelmaraUITheme::MakeSliderTrackBrush(const FLinearColor& Tint)
{
	return MakeBoxBrush(LoadTexture(VelmaraUIPaths::SliderTrack), FMargin(0.03125f, 0.0f, 0.03125f, 0.0f), Tint, 0.5f);
}

FText FVelmaraUITheme::FormatLevelDisplayName(const FString& RawLevelName)
{
	if (RawLevelName.Contains(TEXT("MainLevel"), ESearchCase::IgnoreCase))
	{
		return NSLOCTEXT("VelmaraUI", "LevelMain", "CATEDRAL DEL ECLIPSE");
	}
	if (RawLevelName.Contains(TEXT("Felipe"), ESearchCase::IgnoreCase))
	{
		return NSLOCTEXT("VelmaraUI", "LevelPractice", "ZONA DE PRÁCTICA");
	}
	if (RawLevelName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase))
	{
		return NSLOCTEXT("VelmaraUI", "LevelMainMenu", "MENÚ PRINCIPAL");
	}
	if (RawLevelName.IsEmpty())
	{
		return NSLOCTEXT("VelmaraUI", "LevelUnknown", "DOMINIO DESCONOCIDO");
	}
	return FText::FromString(RawLevelName.ToUpper());
}

FString FVelmaraUITheme::FormatSlotRomanNumeral(int32 SlotIndex)
{
	switch (SlotIndex)
	{
	case 0:
		return TEXT("I");
	case 1:
		return TEXT("II");
	case 2:
		return TEXT("III");
	default:
		return FString::FromInt(SlotIndex + 1);
	}
}

FText FVelmaraUITheme::FormatGroupedNumber(int32 Value)
{
	const bool bNegative = Value < 0;
	FString Digits = FString::FromInt(FMath::Abs(Value));
	FString Formatted;
	int32 Count = 0;
	for (int32 Index = Digits.Len() - 1; Index >= 0; --Index)
	{
		if (Count > 0 && (Count % 3) == 0)
		{
			Formatted.InsertAt(0, TEXT('.'));
		}
		Formatted.InsertAt(0, Digits[Index]);
		++Count;
	}
	if (bNegative)
	{
		Formatted.InsertAt(0, TEXT('-'));
	}
	return FText::FromString(Formatted);
}
