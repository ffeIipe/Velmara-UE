#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"

class UTexture2D;
class UFont;
class UDataTable;
class UCommonUIInputData;
class UCommonButtonBase;
class USoundBase;
struct FButtonStyle;

namespace VelmaraUIPaths
{
	inline constexpr const TCHAR* FontDisplayPrimary = TEXT("/Game/Fonts/Forum_Font.Forum_Font");
	inline constexpr const TCHAR* FontDisplayFallback = TEXT("/Game/Fonts/Cinzel_Font.Cinzel_Font");
	inline constexpr const TCHAR* FontBodyPrimary = TEXT("/Game/Fonts/CARDO-REGULAR_Font.CARDO-REGULAR_Font");
	inline constexpr const TCHAR* FontBodyFallback = TEXT("/Game/NewFonts/Cardo-Regular.Cardo-Regular");
	inline constexpr const TCHAR* FontBodyBold = TEXT("/Game/Fonts/CARDO-BOLD_Font.CARDO-BOLD_Font");
	inline constexpr const TCHAR* FontBodyItalic = TEXT("/Game/Fonts/CARDO-ITALIC_Font.CARDO-ITALIC_Font");
	inline constexpr const TCHAR* FontBodyItalicFallback = TEXT("/Game/NewFonts/Cardo-Italic.Cardo-Italic");

	inline constexpr const TCHAR* InputDataClass = TEXT("/Game/Blueprints/HUD/InputPrompts/BP_VelmaraInputData.BP_VelmaraInputData_C");
	inline constexpr const TCHAR* InputActionsTable = TEXT("/Game/Blueprints/HUD/InputPrompts/DT_VelmaraInputActions.DT_VelmaraInputActions");
	inline constexpr const TCHAR* RichTextStyleSet = TEXT("/Game/Blueprints/DataTables/DT_RichTextStyleSet.DT_RichTextStyleSet");
	inline constexpr const TCHAR* RichImageRowTable = TEXT("/Game/Blueprints/DataTables/DT_RichImageRow.DT_RichImageRow");
	inline constexpr const TCHAR* ShopItemDataTable = TEXT("/Game/Blueprints/DataTables/DT_ShopItemData.DT_ShopItemData");
	inline constexpr const TCHAR* InputPromptsTable = TEXT("/Game/Blueprints/DataTables/DT_InputPrompts.DT_InputPrompts");

	inline constexpr const TCHAR* FrameBracketsPanel = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_Brackets_Panel.T_UI_Frame_Brackets_Panel");
	inline constexpr const TCHAR* FrameBracketsMinimal = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_Brackets_Minimal.T_UI_Frame_Brackets_Minimal");
	inline constexpr const TCHAR* FrameOptionRow = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_OptionRow.T_UI_Frame_OptionRow");
	inline constexpr const TCHAR* FrameOptionRowWithStar = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_OptionRow_WithStar.T_UI_Frame_OptionRow_WithStar");
	inline constexpr const TCHAR* FramePanelGothic = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_Panel_Gothic.T_UI_Frame_Panel_Gothic");
	inline constexpr const TCHAR* FrameSidebarActive = TEXT("/Game/UI/Textures/Frames/T_UI_Frame_Sidebar_Active.T_UI_Frame_Sidebar_Active");
	inline constexpr const TCHAR* BtnHex3Slice = TEXT("/Game/UI/Textures/Frames/T_UI_Btn_Hex_3Slice.T_UI_Btn_Hex_3Slice");
	inline constexpr const TCHAR* SlotSquareNormal = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Square_Normal.T_UI_Slot_Square_Normal");
	inline constexpr const TCHAR* SlotSquareSelected = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Square_Selected.T_UI_Slot_Square_Selected");
	inline constexpr const TCHAR* SlotSquareFillMask = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Square_FillMask.T_UI_Slot_Square_FillMask");
	inline constexpr const TCHAR* SlotDiamondNormal = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Diamond_Normal.T_UI_Slot_Diamond_Normal");
	inline constexpr const TCHAR* SlotDiamondSelected = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Diamond_Selected.T_UI_Slot_Diamond_Selected");
	inline constexpr const TCHAR* SlotDiamondFillMask = TEXT("/Game/UI/Textures/Frames/T_UI_Slot_Diamond_FillMask.T_UI_Slot_Diamond_FillMask");

	inline constexpr const TCHAR* BarHealthFrame = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Health_Frame.T_UI_Bar_Health_Frame");
	inline constexpr const TCHAR* BarHealthFillMask = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Health_FillMask.T_UI_Bar_Health_FillMask");
	inline constexpr const TCHAR* BarShieldFrame = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Shield_Frame.T_UI_Bar_Shield_Frame");
	inline constexpr const TCHAR* BarShieldFillMask = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Shield_FillMask.T_UI_Bar_Shield_FillMask");
	inline constexpr const TCHAR* BarBossFrame = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Boss_Frame.T_UI_Bar_Boss_Frame");
	inline constexpr const TCHAR* BarBossFillMask = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Boss_FillMask.T_UI_Bar_Boss_FillMask");
	inline constexpr const TCHAR* BarFinialSimpleL = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Finial_Simple_L.T_UI_Bar_Finial_Simple_L");
	inline constexpr const TCHAR* BarFinialSimpleR = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Finial_Simple_R.T_UI_Bar_Finial_Simple_R");
	inline constexpr const TCHAR* BarFinialBossL = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Finial_Boss_L.T_UI_Bar_Finial_Boss_L");
	inline constexpr const TCHAR* BarFinialBossR = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_Finial_Boss_R.T_UI_Bar_Finial_Boss_R");
	inline constexpr const TCHAR* BarPhaseMarker = TEXT("/Game/UI/Textures/Bars/T_UI_Bar_PhaseMarker.T_UI_Bar_PhaseMarker");
	inline constexpr const TCHAR* PipDiamondHollow = TEXT("/Game/UI/Textures/Bars/T_UI_Pip_Diamond_Hollow.T_UI_Pip_Diamond_Hollow");
	inline constexpr const TCHAR* PipDiamondFilled = TEXT("/Game/UI/Textures/Bars/T_UI_Pip_Diamond_Filled.T_UI_Pip_Diamond_Filled");
	inline constexpr const TCHAR* RingProgress = TEXT("/Game/UI/Textures/Bars/T_UI_Ring_Progress.T_UI_Ring_Progress");
	inline constexpr const TCHAR* SliderTrack = TEXT("/Game/UI/Textures/Bars/T_UI_Slider_Track.T_UI_Slider_Track");
	inline constexpr const TCHAR* SliderThumb = TEXT("/Game/UI/Textures/Bars/T_UI_Slider_Thumb.T_UI_Slider_Thumb");

	inline constexpr const TCHAR* GradHFadeLeftToRight = TEXT("/Game/UI/Textures/Gradients/T_UI_Grad_HFade_LeftToRight.T_UI_Grad_HFade_LeftToRight");
	inline constexpr const TCHAR* GradHFadeRightToLeft = TEXT("/Game/UI/Textures/Gradients/T_UI_Grad_HFade_RightToLeft.T_UI_Grad_HFade_RightToLeft");
	inline constexpr const TCHAR* GradHFadeCenter = TEXT("/Game/UI/Textures/Gradients/T_UI_Grad_HFade_Center.T_UI_Grad_HFade_Center");
	inline constexpr const TCHAR* GradVFadeBottomUp = TEXT("/Game/UI/Textures/Gradients/T_UI_Grad_VFade_BottomUp.T_UI_Grad_VFade_BottomUp");
	inline constexpr const TCHAR* GradVignetteRadial = TEXT("/Game/UI/Textures/Gradients/T_UI_Grad_Vignette_Radial.T_UI_Grad_Vignette_Radial");

	inline constexpr const TCHAR* OrnChevronL = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Chevron_L.T_UI_Orn_Chevron_L");
	inline constexpr const TCHAR* OrnChevronR = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Chevron_R.T_UI_Orn_Chevron_R");
	inline constexpr const TCHAR* OrnChevronDown = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Chevron_Down.T_UI_Orn_Chevron_Down");
	inline constexpr const TCHAR* OrnRowStar = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_RowStar.T_UI_Orn_RowStar");
	inline constexpr const TCHAR* OrnSepDiamond = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Sep_Diamond.T_UI_Orn_Sep_Diamond");
	inline constexpr const TCHAR* OrnSepStar = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Sep_Star.T_UI_Orn_Sep_Star");
	inline constexpr const TCHAR* OrnSidebarLineV = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_SidebarLine_V.T_UI_Orn_SidebarLine_V");
	inline constexpr const TCHAR* OrnTabActiveLine = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_TabActiveLine.T_UI_Orn_TabActiveLine");
	inline constexpr const TCHAR* OrnTabDivider = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_TabDivider.T_UI_Orn_TabDivider");
	inline constexpr const TCHAR* OrnCrestVelmara = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Crest_Velmara.T_UI_Orn_Crest_Velmara");
	inline constexpr const TCHAR* OrnCrestBoss = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Crest_Boss.T_UI_Orn_Crest_Boss");
	inline constexpr const TCHAR* OrnNodeDiamond = TEXT("/Game/UI/Textures/Ornaments/T_UI_Orn_Node_Diamond.T_UI_Orn_Node_Diamond");
	inline constexpr const TCHAR* MarkerDiamondHollow = TEXT("/Game/UI/Textures/Ornaments/T_UI_Marker_Diamond_Hollow.T_UI_Marker_Diamond_Hollow");
	inline constexpr const TCHAR* MarkerDiamondSolid = TEXT("/Game/UI/Textures/Ornaments/T_UI_Marker_Diamond_Solid.T_UI_Marker_Diamond_Solid");
	inline constexpr const TCHAR* ReticleLockOn = TEXT("/Game/UI/Textures/Ornaments/T_UI_Reticle_LockOn.T_UI_Reticle_LockOn");
	inline constexpr const TCHAR* ReticlePossession = TEXT("/Game/UI/Textures/Ornaments/T_UI_Reticle_Possession.T_UI_Reticle_Possession");
	inline constexpr const TCHAR* SpinnerMoon = TEXT("/Game/UI/Textures/Ornaments/T_UI_Spinner_Moon.T_UI_Spinner_Moon");

	inline constexpr const TCHAR* MaskBloodStrokeLongA = TEXT("/Game/UI/Textures/Masks/T_UI_Mask_BloodStroke_Long_A.T_UI_Mask_BloodStroke_Long_A");
	inline constexpr const TCHAR* MaskBloodStrokeLongB = TEXT("/Game/UI/Textures/Masks/T_UI_Mask_BloodStroke_Long_B.T_UI_Mask_BloodStroke_Long_B");
	inline constexpr const TCHAR* MaskBloodStrokeShortA = TEXT("/Game/UI/Textures/Masks/T_UI_Mask_BloodStroke_Short_A.T_UI_Mask_BloodStroke_Short_A");
	inline constexpr const TCHAR* MaskBloodStrokeShortB = TEXT("/Game/UI/Textures/Masks/T_UI_Mask_BloodStroke_Short_B.T_UI_Mask_BloodStroke_Short_B");
	inline constexpr const TCHAR* BgMainMenu = TEXT("/Game/UI/Textures/Backgrounds/T_UI_Bg_MainMenu.T_UI_Bg_MainMenu");
	inline constexpr const TCHAR* LogoVelmaraV2 = TEXT("/Game/UI/Textures/Backgrounds/T_UI_Logo_Velmara_V2.T_UI_Logo_Velmara_V2");
	inline constexpr const TCHAR* LogoVelmaraLegacy = TEXT("/Game/Images/T_UI_Logo_Velmara.T_UI_Logo_Velmara");
	inline constexpr const TCHAR* LogoVelmaraTitleFallback = TEXT("/Game/Images/TitleScreenOnly.TitleScreenOnly");

	inline constexpr const TCHAR* IconStatBlood = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_Blood.T_UI_Icon_Stat_Blood");
	inline constexpr const TCHAR* IconStatDamage = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_Damage.T_UI_Icon_Stat_Damage");
	inline constexpr const TCHAR* IconStatShield = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_Shield.T_UI_Icon_Stat_Shield");
	inline constexpr const TCHAR* IconStatSpectral = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_Spectral.T_UI_Icon_Stat_Spectral");
	inline constexpr const TCHAR* IconStatSpeed = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_Speed.T_UI_Icon_Stat_Speed");
	inline constexpr const TCHAR* IconStatAttackSpeed = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Stat_AttackSpeed.T_UI_Icon_Stat_AttackSpeed");
	inline constexpr const TCHAR* IconStatusSunlight = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Status_Sunlight.T_UI_Icon_Status_Sunlight");
	inline constexpr const TCHAR* IconStatusDeath = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Status_Death.T_UI_Icon_Status_Death");
	inline constexpr const TCHAR* IconStatusTotem = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Status_Totem.T_UI_Icon_Status_Totem");
	inline constexpr const TCHAR* IconItemVial = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Item_Vial.T_UI_Icon_Item_Vial");
	inline constexpr const TCHAR* IconMenuAltar = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Menu_Altar.T_UI_Icon_Menu_Altar");
	inline constexpr const TCHAR* IconMenuLore = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Menu_Lore.T_UI_Icon_Menu_Lore");
	inline constexpr const TCHAR* IconMenuSkills = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Menu_Skills.T_UI_Icon_Menu_Skills");
	inline constexpr const TCHAR* IconWeaponLongsword = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Weapon_Longsword.T_UI_Icon_Weapon_Longsword");
	inline constexpr const TCHAR* IconWeaponDualBlades = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Weapon_DualBlades.T_UI_Icon_Weapon_DualBlades");
	inline constexpr const TCHAR* IconWeaponSpear = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Weapon_Spear.T_UI_Icon_Weapon_Spear");
	inline constexpr const TCHAR* IconWeaponScythe = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Weapon_Scythe.T_UI_Icon_Weapon_Scythe");
	inline constexpr const TCHAR* IconWeaponFlintlock = TEXT("/Game/UI/Textures/Icons/T_UI_Icon_Weapon_Flintlock.T_UI_Icon_Weapon_Flintlock");

	inline constexpr const TCHAR* SoundMenuHover = TEXT("/Game/Audio/Pato/MetaSounds/UI/MS_MenuHover.MS_MenuHover");
	inline constexpr const TCHAR* SoundMenuClick = TEXT("/Game/Audio/Pato/MetaSounds/UI/MS_MenuClick.MS_MenuClick");
	inline constexpr const TCHAR* AudioBusMaster = TEXT("/Game/Audio/Pato/Modulation/ControlBusses/CB_Master_Vol.CB_Master_Vol");
	inline constexpr const TCHAR* AudioBusMusic = TEXT("/Game/Audio/Pato/Modulation/ControlBusses/CB_Music_Vol.CB_Music_Vol");
	inline constexpr const TCHAR* AudioBusSFX = TEXT("/Game/Audio/Pato/Modulation/ControlBusses/CB_SFX_Vol.CB_SFX_Vol");
	inline constexpr const TCHAR* AudioMixMenuSettings = TEXT("/Game/Audio/Pato/Modulation/ControlBusMixes/CBM_MenuSettings.CBM_MenuSettings");
	inline constexpr const TCHAR* AudioMixPause = TEXT("/Game/Audio/Pato/Modulation/ControlBusMixes/CBM_Pause.CBM_Pause");
	inline constexpr const TCHAR* AudioClassMaster = TEXT("/Game/Audio/Pato/SoundClasses/SC_Master.SC_Master");
	inline constexpr const TCHAR* AudioClassMusic = TEXT("/Game/Audio/Pato/SoundClasses/SC_Music.SC_Music");
	inline constexpr const TCHAR* AudioClassSFX = TEXT("/Game/Audio/Pato/SoundClasses/SC_SFX.SC_SFX");
	inline constexpr const TCHAR* AudioClassUI = TEXT("/Game/Audio/Pato/SoundClasses/SC_UI.SC_UI");
}

struct TESISUE_API FVelmaraUITheme
{
	static FLinearColor Void(float Alpha = 1.0f);
	static FLinearColor Charcoal(float Alpha = 1.0f);
	static FLinearColor Ash(float Alpha = 1.0f);
	static FLinearColor Bone(float Alpha = 1.0f);
	static FLinearColor BoneMuted(float Alpha = 1.0f);
	static FLinearColor Blood(float Alpha = 1.0f);
	static FLinearColor BloodBright(float Alpha = 1.0f);
	static FLinearColor BloodDried(float Alpha = 1.0f);
	static FLinearColor Shield(float Alpha = 1.0f);
	static FLinearColor Spectral(float Alpha = 1.0f);
	static FLinearColor Sunlight(float Alpha = 1.0f);

	static FLinearColor WithAlpha(const FLinearColor& Color, float Alpha);

	static UFont* LoadDisplayFont();
	static UFont* LoadBodyFont();
	static UFont* LoadBoldBodyFont();
	static UFont* LoadItalicFont();

	static FSlateFontInfo GetDisplayFont(int32 Size, int32 LetterSpacing = 180);
	static FSlateFontInfo GetBodyFont(int32 Size, int32 LetterSpacing = 0);
	static FSlateFontInfo GetBoldBodyFont(int32 Size, int32 LetterSpacing = 0);
	static FSlateFontInfo GetItalicFont(int32 Size, int32 LetterSpacing = 10);
	static int32 GetSubtitleFontSize(int32 SubtitleSizeIndex);

	static UTexture2D* LoadTexture(const TCHAR* AssetPath);
	static UTexture2D* LoadVelmaraLogo();

	static void ApplyButtonSounds(UCommonButtonBase& Button, bool bWithClick = true);
	static void ApplyButtonSounds(FButtonStyle& Style);
	static void PlayHoverSound();
	static void PlayClickSound();
	static UDataTable* LoadInputActionsTable();
	static UDataTable* LoadRichTextStyleSet();
	static UDataTable* LoadShopItemDataTable();
	static UDataTable* LoadInputPromptsTable();

	static FSlateBrush MakeSolidBrush(const FLinearColor& Color);
	static FSlateBrush MakeImageBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White);
	static FSlateBrush MakeBorderBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint = FLinearColor::White, float ScaleFactor = 0.5f);
	static FSlateBrush MakeBoxBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint = FLinearColor::White, float ScaleFactor = 0.5f);

	static FSlateBrush MakeOptionRowBrush(const FLinearColor& Tint);
	static FSlateBrush MakeBracketsPanelBrush(const FLinearColor& Tint);
	static FSlateBrush MakeBracketsMinimalBrush(const FLinearColor& Tint);
	static FSlateBrush MakeGothicPanelBrush(const FLinearColor& Tint);
	static FSlateBrush MakeHexButtonBrush(const FLinearColor& Tint);
	static FSlateBrush MakeSquareSlotNormalBrush(const FLinearColor& Tint);
	static FSlateBrush MakeSquareSlotSelectedBrush(const FLinearColor& Tint);
	static FSlateBrush MakeSquareSlotFillBrush(const FLinearColor& Tint);
	static FSlateBrush MakeHealthBarFrameBrush(const FLinearColor& Tint);
	static FSlateBrush MakeShieldBarFrameBrush(const FLinearColor& Tint);
	static FSlateBrush MakeBossBarFrameBrush(const FLinearColor& Tint);
	static FSlateBrush MakeSliderTrackBrush(const FLinearColor& Tint);

	static FText FormatLevelDisplayName(const FString& RawLevelName);
	static FString FormatSlotRomanNumeral(int32 SlotIndex);
	static FText FormatGroupedNumber(int32 Value);
};
