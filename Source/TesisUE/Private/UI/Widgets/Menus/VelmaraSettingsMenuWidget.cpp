#include "UI/Widgets/Menus/VelmaraSettingsMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraConfirmModalWidget.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"
#include "UI/Widgets/Common/VelmaraSettingCarouselRow.h"
#include "UI/Widgets/Common/VelmaraSettingSliderRow.h"

namespace
{
	FVelmaraSettingMetadata MakeMeta(const FText& Title, const FText& Desc, const FText& Quote, bool bRequiresApply = false)
	{
		FVelmaraSettingMetadata Meta;
		Meta.Title = Title;
		Meta.Description = Desc;
		Meta.Quote = Quote;
		Meta.bRequiresApply = bRequiresApply;
		return Meta;
	}

	TArray<FText> MakeQualityOptions4()
	{
		return {
			NSLOCTEXT("VelmaraUI", "QualLow", "BAJA"),
			NSLOCTEXT("VelmaraUI", "QualMed", "MEDIA"),
			NSLOCTEXT("VelmaraUI", "QualHigh", "ALTA"),
			NSLOCTEXT("VelmaraUI", "QualEpic", "ÉPICA")
		};
	}

	TArray<FText> MakeQualityOptions5()
	{
		return {
			NSLOCTEXT("VelmaraUI", "QualLow", "BAJA"),
			NSLOCTEXT("VelmaraUI", "QualMed", "MEDIA"),
			NSLOCTEXT("VelmaraUI", "QualHigh", "ALTA"),
			NSLOCTEXT("VelmaraUI", "QualEpic", "ÉPICA"),
			NSLOCTEXT("VelmaraUI", "QualCustom", "PERSONALIZADA")
		};
	}

	TArray<FText> MakeOffOnOptions()
	{
		return {
			NSLOCTEXT("VelmaraUI", "OptOff", "DESACTIVADA"),
			NSLOCTEXT("VelmaraUI", "OptOn", "ACTIVADA")
		};
	}
}

UVelmaraSettingsMenuWidget::UVelmaraSettingsMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
	FpsOptions = { 30, 60, 90, 120, 0 };
}

TOptional<FUIInputConfig> UVelmaraSettingsMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraSettingsMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraSettingsMenuWidget::PopulateSupportedResolutions()
{
	SupportedResolutions.Reset();
	TArray<FIntPoint> SystemResolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(SystemResolutions);

	for (const FIntPoint& Res : SystemResolutions)
	{
		if (Res.X >= 1280 && Res.Y >= 720 && !SupportedResolutions.Contains(Res))
		{
			SupportedResolutions.Add(Res);
		}
	}

	if (SupportedResolutions.Num() == 0)
	{
		SupportedResolutions = {
			FIntPoint(1280, 720),
			FIntPoint(1600, 900),
			FIntPoint(1920, 1080),
			FIntPoint(2560, 1440),
			FIntPoint(3840, 2160)
		};
	}

	SupportedResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return (A.X != B.X) ? (A.X < B.X) : (A.Y < B.Y);
	});
}

UVerticalBox* UVelmaraSettingsMenuWidget::CreateScrollPage(UWidgetSwitcher* Switcher)
{
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Scroll->SetScrollBarVisibility(ESlateVisibility::Hidden);
	Scroll->SetAnimateWheelScrolling(true);
	Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);

	UVerticalBox* InnerBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(InnerBox);
	Switcher->AddChild(Scroll);
	return InnerBox;
}

UVelmaraSettingCarouselRow* UVelmaraSettingsMenuWidget::AddCarouselRow(
	UVerticalBox* Container,
	const FText& Label,
	const TArray<FText>& Options,
	int32 InitialIndex,
	bool bWrap,
	const FVelmaraSettingMetadata& Meta,
	TFunction<void(int32)> OnChanged)
{
	UVelmaraSettingCarouselRow* Row = WidgetTree->ConstructWidget<UVelmaraSettingCarouselRow>(UVelmaraSettingCarouselRow::StaticClass());
	Row->SetupRow(Label, Options, InitialIndex, bWrap, Meta);
	Row->OnSelectionChanged.AddLambda(MoveTemp(OnChanged));
	Row->OnRowHighlighted.AddUObject(this, &UVelmaraSettingsMenuWidget::UpdateDescriptionPanel);
	if (UVerticalBoxSlot* RowSlot = Container->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	return Row;
}

UVelmaraSettingSliderRow* UVelmaraSettingsMenuWidget::AddSliderRow(
	UVerticalBox* Container,
	const FText& Label,
	float MinVal,
	float MaxVal,
	float Step,
	float InitialVal,
	int32 FormatEnumIndex,
	const FVelmaraSettingMetadata& Meta,
	TFunction<void(float)> OnChanged)
{
	UVelmaraSettingSliderRow* Row = WidgetTree->ConstructWidget<UVelmaraSettingSliderRow>(UVelmaraSettingSliderRow::StaticClass());
	Row->SetupRow(Label, MinVal, MaxVal, Step, InitialVal, static_cast<EVelmaraSliderDisplayFormat>(FormatEnumIndex), Meta);
	Row->OnValueChanged.AddLambda(MoveTemp(OnChanged));
	Row->OnRowHighlighted.AddUObject(this, &UVelmaraSettingsMenuWidget::UpdateDescriptionPanel);
	if (UVerticalBoxSlot* RowSlot = Container->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	return Row;
}

void UVelmaraSettingsMenuWidget::BuildScreenTree()
{
	PopulateSupportedResolutions();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SettingsRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("SettingsBlur"));
	Blur->SetBlurStrength(12.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.78f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* LeftSpineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsLeftSpineSize"));
	LeftSpineSize->SetWidthOverride(20.0f);
	UImage* LeftSpine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsLeftSpine"));
	LeftSpine->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LeftSpine->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSidebarLineV),
		FVector2D(20.0f, 720.0f),
		FVelmaraUITheme::Bone(0.65f)));
	LeftSpineSize->SetContent(LeftSpine);
	if (UOverlaySlot* SpineSlot = RootOverlay->AddChildToOverlay(LeftSpineSize))
	{
		SpineSlot->SetHorizontalAlignment(HAlign_Left);
		SpineSlot->SetVerticalAlignment(VAlign_Fill);
		SpineSlot->SetPadding(FMargin(28.0f, 64.0f, 0.0f, 96.0f));
	}

	USizeBox* TopCrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsCrestSize"));
	TopCrestSize->SetWidthOverride(34.0f);
	TopCrestSize->SetHeightOverride(96.0f);
	UImage* TopCrest = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsCrest"));
	TopCrest->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(34.0f, 96.0f),
		FVelmaraUITheme::BloodBright(0.90f)));
	TopCrestSize->SetContent(TopCrest);
	if (UOverlaySlot* CrestSlot = RootOverlay->AddChildToOverlay(TopCrestSize))
	{
		CrestSlot->SetHorizontalAlignment(HAlign_Right);
		CrestSlot->SetVerticalAlignment(VAlign_Top);
		CrestSlot->SetPadding(FMargin(0.0f, 40.0f, 60.0f, 0.0f));
	}

	UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsMainVBox"));
	if (UOverlaySlot* MainSlot = RootOverlay->AddChildToOverlay(MainVBox))
	{
		MainSlot->SetHorizontalAlignment(HAlign_Fill);
		MainSlot->SetVerticalAlignment(VAlign_Fill);
		MainSlot->SetPadding(FMargin(68.0f, 46.0f, 68.0f, 38.0f));
	}

	UCommonTextBlock* HeaderText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SettingsHeader"));
	HeaderText->SetText(NSLOCTEXT("VelmaraUI", "SettingsHeader", "A J U S T E S"));
	HeaderText->SetFont(FVelmaraUITheme::GetDisplayFont(42, 260));
	HeaderText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* HeadSlot = MainVBox->AddChildToVerticalBox(HeaderText))
	{
		HeadSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* HeadSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsHeadSepSize"));
	HeadSepSize->SetWidthOverride(420.0f);
	HeadSepSize->SetHeightOverride(24.0f);
	UImage* HeadSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsHeadSep"));
	HeadSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.0f, 24.0f),
		FVelmaraUITheme::Bone(0.85f)));
	HeadSepSize->SetContent(HeadSep);
	if (UVerticalBoxSlot* SepSlot = MainVBox->AddChildToVerticalBox(HeadSepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
	}

	UGridPanel* ColumnsGrid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("SettingsColumnsGrid"));
	ColumnsGrid->SetColumnFill(0, 0.22f);
	ColumnsGrid->SetColumnFill(1, 0.50f);
	ColumnsGrid->SetColumnFill(2, 0.28f);
	if (UVerticalBoxSlot* GridSlot = MainVBox->AddChildToVerticalBox(ColumnsGrid))
	{
		GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* SidebarBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsSidebarBox"));
	if (UGridSlot* SideCell = ColumnsGrid->AddChildToGrid(SidebarBox, 0, 0))
	{
		SideCell->SetHorizontalAlignment(HAlign_Fill);
		SideCell->SetVerticalAlignment(VAlign_Top);
		SideCell->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
	}

	const TArray<FText> CategoryLabels = {
		NSLOCTEXT("VelmaraUI", "CatDisplay", "PANTALLA"),
		NSLOCTEXT("VelmaraUI", "CatAudio", "SONIDO"),
		NSLOCTEXT("VelmaraUI", "CatGameplay", "JUEGO"),
		NSLOCTEXT("VelmaraUI", "CatControls", "CONTROLES"),
		NSLOCTEXT("VelmaraUI", "CatAccessibility", "ACCESIBILIDAD"),
		NSLOCTEXT("VelmaraUI", "CatLanguage", "IDIOMA"),
		NSLOCTEXT("VelmaraUI", "CatCredits", "CRÉDITOS")
	};

	CategoryButtons.Reset();
	for (int32 Index = 0; Index < CategoryLabels.Num(); ++Index)
	{
		UVelmaraButtonBase* CatBtn = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass());
		CatBtn->SetVariant(EVelmaraButtonVariant::SidebarTab);
		CatBtn->SetFontSize(19);
		CatBtn->SetButtonMinSize(0.0f, 50.0f);
		CatBtn->SetButtonText(CategoryLabels[Index]);
		CatBtn->OnClicked().AddLambda([this, Index]() { SelectCategory(Index); });
		if (UVerticalBoxSlot* CatSlot = SidebarBox->AddChildToVerticalBox(CatBtn))
		{
			CatSlot->SetHorizontalAlignment(HAlign_Fill);
			CatSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		CategoryButtons.Add(CatBtn);
	}

	CategorySwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("SettingsCategorySwitcher"));
	if (UGridSlot* CenterCell = ColumnsGrid->AddChildToGrid(CategorySwitcher, 0, 1))
	{
		CenterCell->SetHorizontalAlignment(HAlign_Fill);
		CenterCell->SetVerticalAlignment(VAlign_Fill);
		CenterCell->SetPadding(FMargin(0.0f, 0.0f, 32.0f, 0.0f));
	}

	CategoryFirstFocusTargets.Reset();
	CategoryFirstFocusTargets.SetNum(7);

	BuildDisplayPage(CreateScrollPage(CategorySwitcher));
	BuildAudioPage(CreateScrollPage(CategorySwitcher));
	BuildGameplayPage(CreateScrollPage(CategorySwitcher));
	BuildControlsPage(CreateScrollPage(CategorySwitcher));
	BuildAccessibilityPage(CreateScrollPage(CategorySwitcher));
	BuildLanguagePage(CreateScrollPage(CategorySwitcher));
	BuildCreditsPage(CreateScrollPage(CategorySwitcher));

	DescriptionPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("SettingsDescPanel"));
	DescriptionPanel->SetUseMinimalBrackets(false);
	DescriptionPanel->SetBackdropAlpha(0.76f);
	DescriptionPanel->SetShowSignature(true);
	if (UGridSlot* RightCell = ColumnsGrid->AddChildToGrid(DescriptionPanel, 0, 2))
	{
		RightCell->SetHorizontalAlignment(HAlign_Fill);
		RightCell->SetVerticalAlignment(VAlign_Fill);
		RightCell->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UVerticalBox* DescContent = DescriptionPanel->GetContentBox();

	DescTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SettingsDescTitle"));
	DescTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 200));
	DescTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* TSlot = DescContent->AddChildToVerticalBox(DescTitleText))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* DescSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsDescSepSize"));
	DescSepSize->SetHeightOverride(16.0f);
	UImage* DescSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsDescSep"));
	DescSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(300.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.75f)));
	DescSepSize->SetContent(DescSep);
	if (UVerticalBoxSlot* SepS = DescContent->AddChildToVerticalBox(DescSepSize))
	{
		SepS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	DescBodyText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SettingsDescBody"));
	DescBodyText->SetFont(FVelmaraUITheme::GetBodyFont(18, 0));
	DescBodyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	DescBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BSlot = DescContent->AddChildToVerticalBox(DescBodyText))
	{
		BSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	PendingWarningText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SettingsPendingWarn"));
	PendingWarningText->SetText(NSLOCTEXT("VelmaraUI", "PendingGraphicsWarn", "HAY CAMBIOS GRÁFICOS SIN APLICAR"));
	PendingWarningText->SetFont(FVelmaraUITheme::GetDisplayFont(14, 160));
	PendingWarningText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	PendingWarningText->SetAutoWrapText(true);
	PendingWarningText->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* WSlot = DescContent->AddChildToVerticalBox(PendingWarningText))
	{
		WSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	USpacer* DescSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("SettingsDescSpacer"));
	if (UVerticalBoxSlot* SpSlot = DescContent->AddChildToVerticalBox(DescSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	DescQuoteText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SettingsDescQuote"));
	DescQuoteText->SetFont(FVelmaraUITheme::GetItalicFont(17, 10));
	DescQuoteText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.75f)));
	DescQuoteText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* QSlot = DescContent->AddChildToVerticalBox(DescQuoteText))
	{
		QSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	ApplyChangesButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("SettingsApplyBtn"));
	ApplyChangesButton->SetVariant(EVelmaraButtonVariant::HexAction);
	ApplyChangesButton->SetFontSize(16);
	ApplyChangesButton->SetButtonMinSize(0.0f, 46.0f);
	ApplyChangesButton->SetButtonText(NSLOCTEXT("VelmaraUI", "BtnApplyChanges", "APLICAR CAMBIOS"));
	ApplyChangesButton->OnClicked().AddUObject(this, &UVelmaraSettingsMenuWidget::ApplyPendingGraphicsChanges);
	if (UVerticalBoxSlot* AppSlot = DescContent->AddChildToVerticalBox(ApplyChangesButton))
	{
		AppSlot->SetHorizontalAlignment(HAlign_Fill);
		AppSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	ResetDefaultsButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("SettingsResetBtn"));
	ResetDefaultsButton->SetVariant(EVelmaraButtonVariant::HexAction);
	ResetDefaultsButton->SetFontSize(15);
	ResetDefaultsButton->SetButtonMinSize(0.0f, 44.0f);
	ResetDefaultsButton->SetButtonText(NSLOCTEXT("VelmaraUI", "BtnResetDefaults", "RESTABLECER"));
	ResetDefaultsButton->OnClicked().AddUObject(this, &UVelmaraSettingsMenuWidget::RequestResetToDefaults);
	if (UVerticalBoxSlot* RstSlot = DescContent->AddChildToVerticalBox(ResetDefaultsButton))
	{
		RstSlot->SetHorizontalAlignment(HAlign_Fill);
		RstSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 36.0f));
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("SettingsFooterBar"));
	if (UVerticalBoxSlot* FootSlot = MainVBox->AddChildToVerticalBox(FooterBar))
	{
		FootSlot->SetHorizontalAlignment(HAlign_Fill);
		FootSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
	}

	RefreshAllFromGameInstance();
	SelectCategory(0);
}

void UVelmaraSettingsMenuWidget::BuildDisplayPage(UVerticalBox* PageBox)
{
	const TArray<FText> WindowModes = {
		NSLOCTEXT("VelmaraUI", "WinFullscreen", "PANTALLA COMPLETA"),
		NSLOCTEXT("VelmaraUI", "WinBorderless", "VENTANA SIN BORDES"),
		NSLOCTEXT("VelmaraUI", "WinWindowed", "VENTANA")
	};

	WindowModeRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptWindowMode", "MODO DE VENTANA"),
		WindowModes,
		1,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaWinTitle", "MODO DE VENTANA"),
			NSLOCTEXT("VelmaraUI", "MetaWinDesc", "Define cómo se presenta Velmara en tu monitor. Ventana sin bordes permite cambiar rápidamente de aplicación sin parpadeos."),
			NSLOCTEXT("VelmaraUI", "MetaWinQuote", "\"El marco apenas contiene la oscuridad que se desborda.\""),
			true),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			StagedGraphics.WindowMode = (NewIdx == 0) ? EWindowMode::Fullscreen : ((NewIdx == 1) ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
			UpdateDirtyState();
		});

	TArray<FText> ResTexts;
	for (const FIntPoint& Res : SupportedResolutions)
	{
		ResTexts.Add(FText::FromString(FString::Printf(TEXT("%d × %d"), Res.X, Res.Y)));
	}
	ResolutionRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptResolution", "RESOLUCIÓN"),
		ResTexts,
		FMath::Max(0, ResTexts.Num() - 1),
		false,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaResTitle", "RESOLUCIÓN"),
			NSLOCTEXT("VelmaraUI", "MetaResDesc", "Controla la cantidad de píxeles renderizados en pantalla. Resoluciones mayores ofrecen trazos más nítidos a mayor costo de GPU."),
			NSLOCTEXT("VelmaraUI", "MetaResQuote", "\"Cada grieta de la catedral revela un secreto distinto.\""),
			true),
		[this](int32 NewIdx)
		{
			if (bSyncingRows || !SupportedResolutions.IsValidIndex(NewIdx)) return;
			StagedGraphics.ScreenResolution = SupportedResolutions[NewIdx];
			UpdateDirtyState();
		});

	OverallQualityRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptOverall", "CALIDAD GLOBAL"),
		MakeQualityOptions5(),
		2,
		false,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaOverallTitle", "CALIDAD GLOBAL"),
			NSLOCTEXT("VelmaraUI", "MetaOverallDesc", "Ajusta de forma conjunta todas las opciones de escalabilidad gráfica. Modificar un parámetro individual cambiará este valor a Personalizada."),
			NSLOCTEXT("VelmaraUI", "MetaOverallQuote", "\"Un mundo más oscuro siempre revela más.\""),
			true),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (NewIdx <= 3)
			{
				StagedGraphics.ApplyPresetToScalability(NewIdx);
				SyncScalabilityRowsFromStaged();
			}
			else
			{
				StagedGraphics.OverallQualityPreset = 4;
			}
			UpdateDirtyState();
		});

	auto AddScalabilityRow = [&](const FText& Label, const FText& Desc, int32 FVelmaraGraphicsSettingsSnapshot::*FieldPtr) -> UVelmaraSettingCarouselRow*
	{
		return AddCarouselRow(
			PageBox,
			Label,
			MakeQualityOptions4(),
			2,
			false,
			MakeMeta(Label, Desc, NSLOCTEXT("VelmaraUI", "MetaScalQuote", "\"Afila la mirada antes de entrar a la cripta.\""), true),
			[this, FieldPtr](int32 NewIdx)
			{
				if (bSyncingRows) return;
				StagedGraphics.*FieldPtr = FMath::Clamp(NewIdx, 0, 3);
				StagedGraphics.RecalculateOverallPreset();
				bSyncingRows = true;
				if (OverallQualityRow)
				{
					OverallQualityRow->SetSelectedIndex(StagedGraphics.OverallQualityPreset, false);
				}
				bSyncingRows = false;
				UpdateDirtyState();
			});
	};

	ViewDistanceRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptViewDist", "DISTANCIA DE VISIÓN"),
		NSLOCTEXT("VelmaraUI", "DescViewDist", "Distancia máxima a la que se renderizan mallas y detalles arquitectónicos lejanos."),
		&FVelmaraGraphicsSettingsSnapshot::ViewDistanceQuality);

	AntiAliasingRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptAA", "ANTIALIASING"),
		NSLOCTEXT("VelmaraUI", "DescAA", "Suaviza los bordes escalonados de la geometría fina y las siluetas."),
		&FVelmaraGraphicsSettingsSnapshot::AntiAliasingQuality);

	PostProcessRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptPP", "POSPROCESADO"),
		NSLOCTEXT("VelmaraUI", "DescPP", "Controla la precisión del bloom, oclusión ambiental y gradación de color."),
		&FVelmaraGraphicsSettingsSnapshot::PostProcessingQuality);

	ShadowQualityRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptShadow", "CALIDAD DE SOMBRAS"),
		NSLOCTEXT("VelmaraUI", "DescShadow", "Resolución y alcance de las sombras dinámicas proyectadas por antorchas y haces solares."),
		&FVelmaraGraphicsSettingsSnapshot::ShadowQuality);

	GlobalIllumRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptGI", "ILUMINACIÓN GLOBAL"),
		NSLOCTEXT("VelmaraUI", "DescGI", "Precisión de la luz indirecta que rebota en piedra y bóvedas."),
		&FVelmaraGraphicsSettingsSnapshot::GlobalIlluminationQuality);

	VisualEffectsRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptVFX", "EFECTOS VISUALES"),
		NSLOCTEXT("VelmaraUI", "DescVFX", "Densidad de partículas Niagara, neblina de sangre y chispas en combate."),
		&FVelmaraGraphicsSettingsSnapshot::VisualEffectQuality);

	TextureQualityRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptTex", "CALIDAD DE TEXTURAS"),
		NSLOCTEXT("VelmaraUI", "DescTex", "Nivel de detalle y presupuesto de memoria VRAM para texturas del entorno y personajes."),
		&FVelmaraGraphicsSettingsSnapshot::TextureQuality);

	ShadingQualityRow = AddScalabilityRow(
		NSLOCTEXT("VelmaraUI", "OptShading", "SOMBREADO"),
		NSLOCTEXT("VelmaraUI", "DescShading", "Complejidad de materiales, reflejos y sombreado de piel y metales."),
		&FVelmaraGraphicsSettingsSnapshot::ShadingQuality);

	LumenRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptLumen", "ILUMINACIÓN LUMEN"),
		MakeOffOnOptions(),
		1,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaLumenTitle", "ILUMINACIÓN LUMEN"),
			NSLOCTEXT("VelmaraUI", "MetaLumenDesc", "Activa iluminación global y reflejos dinámicos por software Lumen. Desactivarla incrementa notablemente el rendimiento."),
			NSLOCTEXT("VelmaraUI", "MetaLumenQuote", "\"La luz solar quema incluso cuando llega en destellos.\""),
			true),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			StagedGraphics.bLumenEnabled = (NewIdx == 1);
			UpdateDirtyState();
		});

	const TArray<FText> FpsLabels = {
		FText::FromString(TEXT("30")),
		FText::FromString(TEXT("60")),
		FText::FromString(TEXT("90")),
		FText::FromString(TEXT("120")),
		NSLOCTEXT("VelmaraUI", "FpsUnlimited", "SIN LÍMITE")
	};
	MaxFpsRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptMaxFps", "FPS MÁXIMOS"),
		FpsLabels,
		1,
		false,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaFpsTitle", "FPS MÁXIMOS"),
			NSLOCTEXT("VelmaraUI", "MetaFpsDesc", "Limita la tasa de cuadros por segundo para estabilizar el tiempo de cuadro y la temperatura de la GPU."),
			NSLOCTEXT("VelmaraUI", "MetaFpsQuote", "\"La precisión letal exige un pulso constante.\""),
			true),
		[this](int32 NewIdx)
		{
			if (bSyncingRows || !FpsOptions.IsValidIndex(NewIdx)) return;
			StagedGraphics.FrameRateLimit = FpsOptions[NewIdx];
			UpdateDirtyState();
		});

	GammaRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptGamma", "BRILLO / GAMMA"),
		1.4f,
		3.0f,
		0.05f,
		2.2f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Decimal2),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaGammaTitle", "BRILLO / GAMMA"),
			NSLOCTEXT("VelmaraUI", "MetaGammaDesc", "Calibra la luminancia de la imagen en tiempo real sin necesidad de aplicar cambios."),
			NSLOCTEXT("VelmaraUI", "MetaGammaQuote", "\"Distingue al cazador de la sombra antes de que sea tarde.\""),
			false),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetGamma(NewVal);
			}
		});

	CategoryFirstFocusTargets[0] = WindowModeRow;
}

void UVelmaraSettingsMenuWidget::BuildAudioPage(UVerticalBox* PageBox)
{
	MasterVolumeRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptMasterVol", "VOLUMEN GENERAL"),
		0.0f,
		1.0f,
		0.05f,
		1.0f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Percent),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaMasterTitle", "VOLUMEN GENERAL"),
			NSLOCTEXT("VelmaraUI", "MetaMasterDesc", "Ajusta en vivo el volumen maestro de todos los buses y clases de sonido del juego."),
			NSLOCTEXT("VelmaraUI", "MetaMasterQuote", "\"Hasta el silencio de la catedral tiene eco.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetMasterVolume(NewVal);
			}
		});

	MusicVolumeRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptMusicVol", "MÚSICA"),
		0.0f,
		1.0f,
		0.05f,
		0.8f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Percent),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaMusicTitle", "MÚSICA"),
			NSLOCTEXT("VelmaraUI", "MetaMusicDesc", "Controla la intensidad de la banda sonora y los temas de menú y combate en tiempo real."),
			NSLOCTEXT("VelmaraUI", "MetaMusicQuote", "\"Los coros fúnebres anuncian al Paladín.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetMusicVolume(NewVal);
			}
		});

	SFXVolumeRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptSFXVol", "EFECTOS DE SONIDO"),
		0.0f,
		1.0f,
		0.05f,
		1.0f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Percent),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaSFXTitle", "EFECTOS DE SONIDO"),
			NSLOCTEXT("VelmaraUI", "MetaSFXDesc", "Regula el volumen de impactos de espada, habilidades espectrales, voces y ambiente."),
			NSLOCTEXT("VelmaraUI", "MetaSFXQuote", "\"El chasquido del acero avisa antes que el golpe.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetSFXVolume(NewVal);
			}
		});

	UIVolumeRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptUIVol", "INTERFAZ"),
		0.0f,
		1.0f,
		0.05f,
		1.0f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Percent),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaUITitle", "INTERFAZ"),
			NSLOCTEXT("VelmaraUI", "MetaUIDesc", "Controla el volumen de los sonidos de navegación, confirmaciones y alertas de interfaz."),
			NSLOCTEXT("VelmaraUI", "MetaUIQuote", "\"Cada elección queda sellada.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetUIVolume(NewVal);
			}
		});

	CategoryFirstFocusTargets[1] = MasterVolumeRow;
}

void UVelmaraSettingsMenuWidget::BuildGameplayPage(UVerticalBox* PageBox)
{
	VibrationRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptVibration", "VIBRACIÓN DEL MANDO"),
		MakeOffOnOptions(),
		1,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaVibTitle", "VIBRACIÓN DEL MANDO"),
			NSLOCTEXT("VelmaraUI", "MetaVibDesc", "Habilita o desactiva la respuesta háptica del mando durante impactos, esquives y posesiones."),
			NSLOCTEXT("VelmaraUI", "MetaVibQuote", "\"Siente el latido de cada golpe en tus manos.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetVibrationEnabled(NewIdx == 1);
			}
		});

	CameraAssistRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptCamAssist", "ASISTENCIA DE CÁMARA"),
		MakeOffOnOptions(),
		0,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaCamTitle", "ASISTENCIA DE CÁMARA"),
			NSLOCTEXT("VelmaraUI", "MetaCamDesc", "Orienta suavemente la cámara hacia el enemigo objetivo durante los desplazamientos en combate."),
			NSLOCTEXT("VelmaraUI", "MetaCamQuote", "\"Nunca pierdas de vista a tu presa.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetCameraAssist(NewIdx == 1);
			}
		});

	CategoryFirstFocusTargets[2] = VibrationRow;
}

void UVelmaraSettingsMenuWidget::BuildControlsPage(UVerticalBox* PageBox)
{
	MouseSensRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptMouseSens", "SENSIBILIDAD DE RATÓN"),
		0.2f,
		3.0f,
		0.05f,
		1.0f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Decimal2),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaMouseTitle", "SENSIBILIDAD DE RATÓN"),
			NSLOCTEXT("VelmaraUI", "MetaMouseDesc", "Velocidad de giro de la cámara al jugar con ratón y teclado. Se aplica al instante."),
			NSLOCTEXT("VelmaraUI", "MetaMouseQuote", "\"Un giro ágil decide quién golpea primero.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetMouseSensitivity(NewVal);
			}
		});

	GamepadSensRow = AddSliderRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptPadSens", "SENSIBILIDAD DE MANDO"),
		0.2f,
		3.0f,
		0.05f,
		1.0f,
		static_cast<int32>(EVelmaraSliderDisplayFormat::Decimal2),
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaPadTitle", "SENSIBILIDAD DE MANDO"),
			NSLOCTEXT("VelmaraUI", "MetaPadDesc", "Velocidad de rotación de la cámara al utilizar el stick derecho del mando."),
			NSLOCTEXT("VelmaraUI", "MetaPadQuote", "\"Domina el ritmo de la cacería.\"")),
		[this](float NewVal)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetGamepadSensitivity(NewVal);
			}
		});

	const TArray<FText> NoYesOptions = {
		NSLOCTEXT("VelmaraUI", "OptNo", "NO"),
		NSLOCTEXT("VelmaraUI", "OptYes", "SÍ")
	};
	InvertYRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptInvertY", "INVERTIR EJE VERTICAL"),
		NoYesOptions,
		0,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaInvTitle", "INVERTIR EJE VERTICAL"),
			NSLOCTEXT("VelmaraUI", "MetaInvDesc", "Invierte la dirección vertical (arriba / abajo) del control de cámara."),
			NSLOCTEXT("VelmaraUI", "MetaInvQuote", "\"El cielo y el abismo intercambian lugar.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetInvertY(NewIdx == 1);
			}
		});

	UCommonTextBlock* BindHeader = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("BindingsSectionHeader"));
	BindHeader->SetText(NSLOCTEXT("VelmaraUI", "BindingsTitle", "ASIGNACIÓN DE CONTROLES"));
	BindHeader->SetFont(FVelmaraUITheme::GetDisplayFont(17, 220));
	BindHeader->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* BHSlot = PageBox->AddChildToVerticalBox(BindHeader))
	{
		BHSlot->SetPadding(FMargin(4.0f, 18.0f, 0.0f, 10.0f));
	}

	BindingsTableBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BindingsTableBox"));
	PageBox->AddChildToVerticalBox(BindingsTableBox);
	PopulateControlBindingsTable();

	CategoryFirstFocusTargets[3] = MouseSensRow;
}

void UVelmaraSettingsMenuWidget::PopulateControlBindingsTable()
{
	if (!BindingsTableBox)
	{
		return;
	}
	BindingsTableBox->ClearChildren();

	struct FControlBindingRowData
	{
		const TCHAR* ActionLabel;
		const TCHAR* DefaultKBM;
		const TCHAR* DefaultGamepad;
	};

	const FControlBindingRowData Rows[] = {
		{ TEXT("ATAQUE"), TEXT("CLIC IZQ"), TEXT("X / CUADRADO") },
		{ TEXT("HABILIDAD 1 (MODO ESPECTRAL)"), TEXT("1 / Q"), TEXT("LB / L1") },
		{ TEXT("HABILIDAD 2 (POSEER)"), TEXT("2 / F"), TEXT("RB / R1") },
		{ TEXT("HABILIDAD 3 (BLOQUEO)"), TEXT("3 / CLIC DER"), TEXT("LT / L2") },
		{ TEXT("HABILIDAD 4"), TEXT("4 / R"), TEXT("RT / R2") },
		{ TEXT("CAMBIAR ARMA"), TEXT("TAB / RUEDA"), TEXT("CRUCETA DER") },
		{ TEXT("ESQUIVAR"), TEXT("SHIFT / ESPACIO"), TEXT("B / CÍRCULO") },
		{ TEXT("FIJAR OBJETIVO"), TEXT("CLIC MEDIO"), TEXT("R3") },
		{ TEXT("INTERACTUAR"), TEXT("E"), TEXT("A / CRUZ") },
		{ TEXT("INVENTARIO"), TEXT("I"), TEXT("SELECT / VIEW") },
		{ TEXT("PAUSA"), TEXT("ESC"), TEXT("START / OPTIONS") }
	};

	for (const FControlBindingRowData& Entry : Rows)
	{
		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBorder->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.55f)));
		RowBorder->SetPadding(FMargin(18.0f, 10.0f));

		UGridPanel* RowGrid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass());
		RowGrid->SetColumnFill(0, 0.48f);
		RowGrid->SetColumnFill(1, 0.26f);
		RowGrid->SetColumnFill(2, 0.26f);
		RowBorder->SetContent(RowGrid);

		UCommonTextBlock* ActionText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		ActionText->SetText(FText::FromString(Entry.ActionLabel));
		ActionText->SetFont(FVelmaraUITheme::GetDisplayFont(15, 140));
		ActionText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		if (UGridSlot* S0 = RowGrid->AddChildToGrid(ActionText, 0, 0))
		{
			S0->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* KBMText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		KBMText->SetText(FText::FromString(Entry.DefaultKBM));
		KBMText->SetFont(FVelmaraUITheme::GetDisplayFont(14, 120));
		KBMText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		KBMText->SetJustification(ETextJustify::Center);
		if (UGridSlot* S1 = RowGrid->AddChildToGrid(KBMText, 0, 1))
		{
			S1->SetHorizontalAlignment(HAlign_Center);
			S1->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* PadText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		PadText->SetText(FText::FromString(Entry.DefaultGamepad));
		PadText->SetFont(FVelmaraUITheme::GetDisplayFont(14, 120));
		PadText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Spectral()));
		PadText->SetJustification(ETextJustify::Center);
		if (UGridSlot* S2 = RowGrid->AddChildToGrid(PadText, 0, 2))
		{
			S2->SetHorizontalAlignment(HAlign_Center);
			S2->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* RSlot = BindingsTableBox->AddChildToVerticalBox(RowBorder))
		{
			RSlot->SetHorizontalAlignment(HAlign_Fill);
			RSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}
	}
}

void UVelmaraSettingsMenuWidget::BuildAccessibilityPage(UVerticalBox* PageBox)
{
	const TArray<FText> SubToggle = {
		NSLOCTEXT("VelmaraUI", "OptSubOff", "DESACTIVADOS"),
		NSLOCTEXT("VelmaraUI", "OptSubOn", "ACTIVADOS")
	};
	SubtitlesEnabledRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptSubtitles", "SUBTÍTULOS"),
		SubToggle,
		1,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaSubTitle", "SUBTÍTULOS"),
			NSLOCTEXT("VelmaraUI", "MetaSubDesc", "Muestra subtítulos de diálogos y narración sobre una banda oscura legible en la parte inferior."),
			NSLOCTEXT("VelmaraUI", "MetaSubQuote", "\"Ningún susurro debe perderse en el viento.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetSubtitlesEnabled(NewIdx == 1);
			}
		});

	const TArray<FText> SubSizes = {
		NSLOCTEXT("VelmaraUI", "SubSmall", "PEQUEÑO"),
		NSLOCTEXT("VelmaraUI", "SubNormal", "NORMAL"),
		NSLOCTEXT("VelmaraUI", "SubLarge", "GRANDE")
	};
	SubtitleSizeRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptSubSize", "TAMAÑO DE SUBTÍTULOS"),
		SubSizes,
		1,
		false,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaSubSizeTitle", "TAMAÑO DE SUBTÍTULOS"),
			NSLOCTEXT("VelmaraUI", "MetaSubSizeDesc", "Ajusta el tamaño tipográfico de los subtítulos (Pequeño 24px, Normal 30px, Grande 38px)."),
			NSLOCTEXT("VelmaraUI", "MetaSubSizeQuote", "\"Claridad incluso en medio del eclipse.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetSubtitleSizeIndex(NewIdx);
			}
		});

	CategoryFirstFocusTargets[4] = SubtitlesEnabledRow;
}

void UVelmaraSettingsMenuWidget::BuildLanguagePage(UVerticalBox* PageBox)
{
	const TArray<FText> Languages = {
		FText::FromString(TEXT("ESPAÑOL")),
		FText::FromString(TEXT("ENGLISH"))
	};
	LanguageRow = AddCarouselRow(
		PageBox,
		NSLOCTEXT("VelmaraUI", "OptLanguage", "IDIOMA DE TEXTOS"),
		Languages,
		0,
		true,
		MakeMeta(
			NSLOCTEXT("VelmaraUI", "MetaLangTitle", "IDIOMA DE TEXTOS"),
			NSLOCTEXT("VelmaraUI", "MetaLangDesc", "Cambia la cultura activa de textos, menús y subtítulos de Velmara."),
			NSLOCTEXT("VelmaraUI", "MetaLangQuote", "\"Las lenguas antiguas conservan el mismo juramento.\"")),
		[this](int32 NewIdx)
		{
			if (bSyncingRows) return;
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
			{
				GI->SetLanguageCode(NewIdx == 1 ? TEXT("en") : TEXT("es"));
			}
		});

	CategoryFirstFocusTargets[5] = LanguageRow;
}

void UVelmaraSettingsMenuWidget::BuildCreditsPage(UVerticalBox* PageBox)
{
	UBorder* CreditsCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreditsCard"));
	CreditsCard->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal(0.60f)));
	CreditsCard->SetPadding(FMargin(32.0f, 28.0f));
	PageBox->AddChildToVerticalBox(CreditsCard);

	UVerticalBox* CreditVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsVBox"));
	CreditsCard->SetContent(CreditVBox);

	UCommonTextBlock* ProjTitle = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
	ProjTitle->SetText(NSLOCTEXT("VelmaraUI", "CreditsProject", "VELMARA — PROYECTO DE TESIS"));
	ProjTitle->SetFont(FVelmaraUITheme::GetDisplayFont(26, 220));
	ProjTitle->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* PSlot = CreditVBox->AddChildToVerticalBox(ProjTitle))
	{
		PSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SepSize->SetHeightOverride(18.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(420.0f, 18.0f),
		FVelmaraUITheme::BloodBright()));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SSlot = CreditVBox->AddChildToVerticalBox(SepSize))
	{
		SSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	auto AddCreditEntry = [&](const TCHAR* Role, const TCHAR* Detail)
	{
		UCommonTextBlock* RoleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		RoleText->SetText(FText::FromString(Role));
		RoleText->SetFont(FVelmaraUITheme::GetDisplayFont(15, 190));
		RoleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
		CreditVBox->AddChildToVerticalBox(RoleText)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));

		UCommonTextBlock* DetailText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		DetailText->SetText(FText::FromString(Detail));
		DetailText->SetFont(FVelmaraUITheme::GetBodyFont(18, 0));
		DetailText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		DetailText->SetAutoWrapText(true);
		CreditVBox->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	};

	AddCreditEntry(TEXT("DESARROLLO Y ARQUITECTURA C++"), TEXT("Equipo Velmara — Tesis Unreal Engine 5"));
	AddCreditEntry(TEXT("SISTEMA DE COMBATE, IA Y GAS"), TEXT("Gameplay Ability System · Motion Warping · Posesión Espectral"));
	AddCreditEntry(TEXT("AUDIO Y DISEÑO SONORO"), TEXT("MetaSounds · Audio Modulation (Buses y Mezclas Dinámicas)"));
	AddCreditEntry(TEXT("INTERFAZ Y EXPERIENCIA DE USUARIO"), TEXT("Common UI · Common Input · Tipografías Forum & Cardo"));
	AddCreditEntry(TEXT("MOTOR Y HERRAMIENTAS"), TEXT("Unreal Engine 5.5 · C++20 · Niagara · Lumen"));

	CategoryFirstFocusTargets[6] = CategoryButtons.IsValidIndex(6) ? CategoryButtons[6].Get() : nullptr;
}

void UVelmaraSettingsMenuWidget::RefreshAllFromGameInstance()
{
	UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	bSyncingRows = true;

	AppliedGraphics = GI->GetCurrentGraphicsSnapshot();
	StagedGraphics = AppliedGraphics;

	if (WindowModeRow)
	{
		const int32 WinIdx = (StagedGraphics.WindowMode == EWindowMode::Fullscreen) ? 0
			: ((StagedGraphics.WindowMode == EWindowMode::WindowedFullscreen) ? 1 : 2);
		WindowModeRow->SetSelectedIndex(WinIdx, false);
	}

	if (ResolutionRow)
	{
		int32 ResIdx = SupportedResolutions.IndexOfByKey(StagedGraphics.ScreenResolution);
		if (ResIdx == INDEX_NONE)
		{
			SupportedResolutions.Add(StagedGraphics.ScreenResolution);
			SupportedResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
			{
				return (A.X != B.X) ? (A.X < B.X) : (A.Y < B.Y);
			});
			TArray<FText> ResTexts;
			for (const FIntPoint& Res : SupportedResolutions)
			{
				ResTexts.Add(FText::FromString(FString::Printf(TEXT("%d × %d"), Res.X, Res.Y)));
			}
			ResIdx = SupportedResolutions.IndexOfByKey(StagedGraphics.ScreenResolution);
			ResolutionRow->SetOptions(ResTexts, FMath::Max(0, ResIdx), false);
		}
		else
		{
			ResolutionRow->SetSelectedIndex(ResIdx, false);
		}
	}

	SyncScalabilityRowsFromStaged();

	if (LumenRow)
	{
		LumenRow->SetSelectedIndex(StagedGraphics.bLumenEnabled ? 1 : 0, false);
	}

	if (MaxFpsRow)
	{
		int32 FpsIdx = FpsOptions.IndexOfByKey(StagedGraphics.FrameRateLimit);
		if (FpsIdx == INDEX_NONE)
		{
			FpsIdx = (StagedGraphics.FrameRateLimit <= 0) ? 4 : 1;
		}
		MaxFpsRow->SetSelectedIndex(FpsIdx, false);
	}

	if (GammaRow) GammaRow->SetSliderValue(GI->GetGamma(), false);
	if (MasterVolumeRow) MasterVolumeRow->SetSliderValue(GI->GetMasterVolume(), false);
	if (MusicVolumeRow) MusicVolumeRow->SetSliderValue(GI->GetMusicVolume(), false);
	if (SFXVolumeRow) SFXVolumeRow->SetSliderValue(GI->GetSFXVolume(), false);
	if (UIVolumeRow) UIVolumeRow->SetSliderValue(GI->GetUIVolume(), false);
	if (VibrationRow) VibrationRow->SetSelectedIndex(GI->IsVibrationEnabled() ? 1 : 0, false);
	if (CameraAssistRow) CameraAssistRow->SetSelectedIndex(GI->IsCameraAssist() ? 1 : 0, false);
	if (MouseSensRow) MouseSensRow->SetSliderValue(GI->GetMouseSensitivity(), false);
	if (GamepadSensRow) GamepadSensRow->SetSliderValue(GI->GetGamepadSensitivity(), false);
	if (InvertYRow) InvertYRow->SetSelectedIndex(GI->IsInvertY() ? 1 : 0, false);
	if (SubtitlesEnabledRow) SubtitlesEnabledRow->SetSelectedIndex(GI->AreSubtitlesEnabled() ? 1 : 0, false);
	if (SubtitleSizeRow) SubtitleSizeRow->SetSelectedIndex(GI->GetSubtitleSizeIndex(), false);
	if (LanguageRow) LanguageRow->SetSelectedIndex(GI->GetLanguageCode().StartsWith(TEXT("en"), ESearchCase::IgnoreCase) ? 1 : 0, false);

	bSyncingRows = false;
	UpdateDirtyState();
}

void UVelmaraSettingsMenuWidget::SyncScalabilityRowsFromStaged()
{
	const bool bPrevSync = bSyncingRows;
	bSyncingRows = true;

	if (OverallQualityRow) OverallQualityRow->SetSelectedIndex(StagedGraphics.OverallQualityPreset, false);
	if (ViewDistanceRow) ViewDistanceRow->SetSelectedIndex(StagedGraphics.ViewDistanceQuality, false);
	if (AntiAliasingRow) AntiAliasingRow->SetSelectedIndex(StagedGraphics.AntiAliasingQuality, false);
	if (PostProcessRow) PostProcessRow->SetSelectedIndex(StagedGraphics.PostProcessingQuality, false);
	if (ShadowQualityRow) ShadowQualityRow->SetSelectedIndex(StagedGraphics.ShadowQuality, false);
	if (GlobalIllumRow) GlobalIllumRow->SetSelectedIndex(StagedGraphics.GlobalIlluminationQuality, false);
	if (VisualEffectsRow) VisualEffectsRow->SetSelectedIndex(StagedGraphics.VisualEffectQuality, false);
	if (TextureQualityRow) TextureQualityRow->SetSelectedIndex(StagedGraphics.TextureQuality, false);
	if (ShadingQualityRow) ShadingQualityRow->SetSelectedIndex(StagedGraphics.ShadingQuality, false);

	bSyncingRows = bPrevSync;
}

void UVelmaraSettingsMenuWidget::SelectCategory(int32 CategoryIndex)
{
	if (!CategorySwitcher || CategoryButtons.Num() == 0)
	{
		return;
	}

	ActiveCategoryIndex = FMath::Clamp(CategoryIndex, 0, CategoryButtons.Num() - 1);
	CategorySwitcher->SetActiveWidgetIndex(ActiveCategoryIndex);

	for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
	{
		if (CategoryButtons[Index])
		{
			CategoryButtons[Index]->SetHighlightedState(Index == ActiveCategoryIndex);
		}
	}

	if (CategoryFirstFocusTargets.IsValidIndex(ActiveCategoryIndex) && CategoryFirstFocusTargets[ActiveCategoryIndex])
	{
		if (UVelmaraSettingCarouselRow* FirstCarousel = Cast<UVelmaraSettingCarouselRow>(CategoryFirstFocusTargets[ActiveCategoryIndex]))
		{
			UpdateDescriptionPanel(FirstCarousel->GetMetadata());
		}
		else if (UVelmaraSettingSliderRow* FirstSlider = Cast<UVelmaraSettingSliderRow>(CategoryFirstFocusTargets[ActiveCategoryIndex]))
		{
			UpdateDescriptionPanel(FirstSlider->GetMetadata());
		}
		else
		{
			UpdateDescriptionPanel(MakeMeta(
				NSLOCTEXT("VelmaraUI", "MetaCredTitle", "CRÉDITOS"),
				NSLOCTEXT("VelmaraUI", "MetaCredDesc", "Velmara es un ARPG gótico de acción desarrollado como proyecto de Tesis en Unreal Engine 5."),
				NSLOCTEXT("VelmaraUI", "MetaCredQuote", "\"Algunas cosas nunca mueren.\"")));
		}
		CategoryFirstFocusTargets[ActiveCategoryIndex]->SetKeyboardFocus();
	}
}

void UVelmaraSettingsMenuWidget::StepCategory(int32 Delta)
{
	if (CategoryButtons.Num() == 0)
	{
		return;
	}
	const int32 NextIdx = (ActiveCategoryIndex + Delta + CategoryButtons.Num()) % CategoryButtons.Num();
	SelectCategory(NextIdx);
}

void UVelmaraSettingsMenuWidget::UpdateDirtyState()
{
	bHasPendingGraphicsChanges = (StagedGraphics != AppliedGraphics);

	if (PendingWarningText)
	{
		PendingWarningText->SetVisibility(bHasPendingGraphicsChanges ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (ApplyChangesButton)
	{
		ApplyChangesButton->SetIsEnabled(bHasPendingGraphicsChanges);
		ApplyChangesButton->SetHighlightedState(bHasPendingGraphicsChanges);
	}
	UpdateFooterActions();
}

void UVelmaraSettingsMenuWidget::UpdateDescriptionPanel(const FVelmaraSettingMetadata& Meta)
{
	if (DescTitleText)
	{
		DescTitleText->SetText(Meta.Title.ToUpper());
	}
	if (DescBodyText)
	{
		DescBodyText->SetText(Meta.Description);
	}
	if (DescQuoteText)
	{
		DescQuoteText->SetText(Meta.Quote);
	}
}

void UVelmaraSettingsMenuWidget::UpdateFooterActions()
{
	if (!FooterBar)
	{
		return;
	}

	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "FooterChange", "CAMBIAR")),
		FVelmaraFooterAction(TEXT("Apply"), NSLOCTEXT("VelmaraUI", "FooterApply", "APLICAR CAMBIOS"), bHasPendingGraphicsChanges, bHasPendingGraphicsChanges),
		FVelmaraFooterAction(TEXT("Reset"), NSLOCTEXT("VelmaraUI", "FooterReset", "RESTABLECER")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "FooterBack", "ATRÁS"))
	});
}

void UVelmaraSettingsMenuWidget::ApplyPendingGraphicsChanges()
{
	if (!bHasPendingGraphicsChanges)
	{
		return;
	}

	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->ApplyStagedGraphicsSettings(StagedGraphics);
		AppliedGraphics = GI->GetCurrentGraphicsSnapshot();
		StagedGraphics = AppliedGraphics;
		UpdateDirtyState();
		if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
		{
			Layout->ShowTransientNotification(NSLOCTEXT("VelmaraUI", "NotifGraphicsApplied", "CAMBIOS GRÁFICOS APLICADOS"));
		}
	}
}

void UVelmaraSettingsMenuWidget::RequestResetToDefaults()
{
	TWeakObjectPtr<UVelmaraSettingsMenuWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		NSLOCTEXT("VelmaraUI", "ResetModalTitle", "¿RESTABLECER AJUSTES?"),
		NSLOCTEXT("VelmaraUI", "ResetModalBody", "Todos los parámetros de pantalla, sonido, juego, controles y accesibilidad volverán a sus valores predeterminados."),
		NSLOCTEXT("VelmaraUI", "ResetModalConfirm", "RESTABLECER"),
		NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
		[WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(WeakThis->GetGameInstance()))
			{
				GI->ResetAllSettingsToDefaults();
			}
			WeakThis->RefreshAllFromGameInstance();
			if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(WeakThis.Get()))
			{
				Layout->ShowTransientNotification(NSLOCTEXT("VelmaraUI", "NotifResetDone", "AJUSTES RESTABLECIDOS"));
			}
		});
}

void UVelmaraSettingsMenuWidget::HandleBackRequested()
{
	if (!bHasPendingGraphicsChanges)
	{
		DeactivateWidget();
		return;
	}

	TWeakObjectPtr<UVelmaraSettingsMenuWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		NSLOCTEXT("VelmaraUI", "PendingExitTitle", "¿APLICAR CAMBIOS GRÁFICOS?"),
		NSLOCTEXT("VelmaraUI", "PendingExitBody", "Hay modificaciones gráficas sin aplicar. ¿Deseas aplicarlas antes de salir o descartarlas?"),
		NSLOCTEXT("VelmaraUI", "PendingExitApply", "APLICAR Y SALIR"),
		NSLOCTEXT("VelmaraUI", "PendingExitDiscard", "DESCARTAR"),
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ApplyPendingGraphicsChanges();
				WeakThis->DeactivateWidget();
			}
		},
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StagedGraphics = WeakThis->AppliedGraphics;
				WeakThis->UpdateDirtyState();
				WeakThis->DeactivateWidget();
			}
		});
}

void UVelmaraSettingsMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshAllFromGameInstance();
	SelectCategory(ActiveCategoryIndex);
}

UWidget* UVelmaraSettingsMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (CategoryFirstFocusTargets.IsValidIndex(ActiveCategoryIndex) && CategoryFirstFocusTargets[ActiveCategoryIndex])
	{
		return CategoryFirstFocusTargets[ActiveCategoryIndex];
	}
	return CategoryButtons.Num() > 0 ? CategoryButtons[0].Get() : nullptr;
}

bool UVelmaraSettingsMenuWidget::NativeOnHandleBackAction()
{
	HandleBackRequested();
	return true;
}

FReply UVelmaraSettingsMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleBackRequested();
		return FReply::Handled();
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		StepCategory(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		StepCategory(1);
		return FReply::Handled();
	}
	if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top)
	{
		if (bHasPendingGraphicsChanges)
		{
			ApplyPendingGraphicsChanges();
		}
		return FReply::Handled();
	}
	if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
	{
		RequestResetToDefaults();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
