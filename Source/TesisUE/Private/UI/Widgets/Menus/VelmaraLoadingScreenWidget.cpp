#include "UI/Widgets/Menus/VelmaraLoadingScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/VelmaraUITheme.h"

UVelmaraLoadingScreenWidget::UVelmaraLoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UVelmaraLoadingScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLoadingTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectRandomTip();
}

void UVelmaraLoadingScreenWidget::BuildLoadingTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LoadingRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* VoidBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingVoidBg"));
	VoidBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(1.0f)));
	if (UOverlaySlot* BgSlot = RootOverlay->AddChildToOverlay(VoidBg))
	{
		BgSlot->SetHorizontalAlignment(HAlign_Fill);
		BgSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UScaleBox* ArtScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("LoadingArtScale"));
	ArtScale->SetStretch(EStretch::ScaleToFill);
	UImage* ArtImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoadingArtImg"));
	ArtImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::BgMainMenu),
		FVector2D(1920.0f, 1080.0f),
		FLinearColor(0.65f, 0.65f, 0.68f, 1.0f)));
	ArtScale->SetContent(ArtImg);
	if (UOverlaySlot* ArtSlot = RootOverlay->AddChildToOverlay(ArtScale))
	{
		ArtSlot->SetHorizontalAlignment(HAlign_Fill);
		ArtSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* BottomGradSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingBottomGradSize"));
	BottomGradSize->SetHeightOverride(440.0f);
	UImage* BottomGrad = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoadingBottomGrad"));
	BottomGrad->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradVFadeBottomUp),
		FVector2D(1920.0f, 440.0f),
		FVelmaraUITheme::Void(1.0f)));
	BottomGradSize->SetContent(BottomGrad);
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(BottomGradSize))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	USizeBox* TipWrapSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingTipWrapSize"));
	TipWrapSize->SetMaxDesiredWidth(840.0f);
	UVerticalBox* TipVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadingTipVBox"));
	TipWrapSize->SetContent(TipVBox);
	if (UOverlaySlot* TipSlot = RootOverlay->AddChildToOverlay(TipWrapSize))
	{
		TipSlot->SetHorizontalAlignment(HAlign_Left);
		TipSlot->SetVerticalAlignment(VAlign_Bottom);
		TipSlot->SetPadding(FMargin(64.0f, 0.0f, 0.0f, 56.0f));
	}

	UCommonTextBlock* TipHeader = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("LoadingTipHeader"));
	TipHeader->SetText(NSLOCTEXT("VelmaraUI", "LoadingTipLabel", "C O N S E J O"));
	TipHeader->SetFont(FVelmaraUITheme::GetDisplayFont(16, 260));
	TipHeader->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* HeaderSlot = TipVBox->AddChildToVerticalBox(TipHeader))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingTipSepSize"));
	SepSize->SetWidthOverride(420.0f);
	SepSize->SetHeightOverride(22.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoadingTipSep"));
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(420.0f, 22.0f),
		FVelmaraUITheme::Bone(0.80f)));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = TipVBox->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	TipBodyText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("LoadingTipBody"));
	TipBodyText->SetFont(FVelmaraUITheme::GetBodyFont(21, 0));
	TipBodyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	TipBodyText->SetAutoWrapText(true);
	TipVBox->AddChildToVerticalBox(TipBodyText);

	UHorizontalBox* SpinnerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LoadingSpinnerRow"));
	if (UOverlaySlot* SpinSlot = RootOverlay->AddChildToOverlay(SpinnerRow))
	{
		SpinSlot->SetHorizontalAlignment(HAlign_Right);
		SpinSlot->SetVerticalAlignment(VAlign_Bottom);
		SpinSlot->SetPadding(FMargin(0.0f, 0.0f, 64.0f, 56.0f));
	}

	USizeBox* SpinnerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingSpinnerSize"));
	SpinnerSize->SetWidthOverride(74.0f);
	SpinnerSize->SetHeightOverride(74.0f);
	SpinnerImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoadingSpinnerImg"));
	SpinnerImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SpinnerImage->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::SpinnerMoon),
		FVector2D(74.0f, 74.0f),
		FVelmaraUITheme::Bone()));
	SpinnerSize->SetContent(SpinnerImage);
	if (UHorizontalBoxSlot* SImgSlot = SpinnerRow->AddChildToHorizontalBox(SpinnerSize))
	{
		SImgSlot->SetVerticalAlignment(VAlign_Center);
		SImgSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	UCommonTextBlock* LoadingLabel = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("LoadingTextLabel"));
	LoadingLabel->SetText(NSLOCTEXT("VelmaraUI", "LoadingLabel", "C A R G A N D O"));
	LoadingLabel->SetFont(FVelmaraUITheme::GetDisplayFont(15, 240));
	LoadingLabel->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	if (UHorizontalBoxSlot* LSlot = SpinnerRow->AddChildToHorizontalBox(LoadingLabel))
	{
		LSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* ProgressSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoadingProgressSize"));
	ProgressSize->SetHeightOverride(3.0f);
	ProgressSize->SetVisibility(ESlateVisibility::Collapsed);
	ProgressContainer = ProgressSize;

	ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("LoadingProgressBar"));
	FProgressBarStyle BarStyle;
	BarStyle.SetBackgroundImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Charcoal()));
	BarStyle.SetFillImage(FVelmaraUITheme::MakeSolidBrush(FLinearColor::White));
	ProgressBar->SetWidgetStyle(BarStyle);
	ProgressBar->SetFillColorAndOpacity(FVelmaraUITheme::BloodBright());
	ProgressBar->SetPercent(0.0f);
	ProgressSize->SetContent(ProgressBar);

	if (UOverlaySlot* ProgressSlot = RootOverlay->AddChildToOverlay(ProgressSize))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
		ProgressSlot->SetVerticalAlignment(VAlign_Bottom);
		ProgressSlot->SetPadding(FMargin(64.0f, 0.0f, 64.0f, 28.0f));
	}

	SelectRandomTip();
}

void UVelmaraLoadingScreenWidget::SelectRandomTip()
{
	if (!TipBodyText)
	{
		return;
	}

	const TArray<FText> Tips = {
		NSLOCTEXT("VelmaraUI", "LoadTip1", "La luz del sol debilita tu forma inmortal. Permanece bajo las sombras de las bóvedas durante el combate."),
		NSLOCTEXT("VelmaraUI", "LoadTip2", "El Sentido Vampírico revela palancas, mecanismos ocultos y enemigos vulnerables a la posesión espectral."),
		NSLOCTEXT("VelmaraUI", "LoadTip3", "Poseer a un enemigo te protege temporalmente de la luz solar, pero consume tu Energía Espectral de forma continua."),
		NSLOCTEXT("VelmaraUI", "LoadTip4", "Ofrece la Sangre recolectada en los Altares para fortalecer tus atributos y despertar nuevos combos de armas."),
		NSLOCTEXT("VelmaraUI", "LoadTip5", "Romper el escudo de un adversario abre una ventana crítica para encadenar remates devastadores."),
		NSLOCTEXT("VelmaraUI", "LoadTip6", "Alterna entre distintas armas para adaptarte al alcance y velocidad de cada guardián de la catedral.")
	};

	const int32 Pick = FMath::RandRange(0, Tips.Num() - 1);
	TipBodyText->SetText(Tips[Pick]);
}

void UVelmaraLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (SpinnerImage)
	{
		SpinnerAngleDeg = FMath::Fmod(SpinnerAngleDeg + 30.0f * InDeltaTime, 360.0f);
		SpinnerImage->SetRenderTransformAngle(SpinnerAngleDeg);
	}
}
