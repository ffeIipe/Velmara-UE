#include "UI/Widgets/Menus/VelmaraBloodAltarMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressionComponent.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"
#include "Features/UpgradeSystem/Core/UpgradeComponentBase.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "Features/WeaponUpgradeSystem/UI/WeaponUpgradeMenuWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CustomPlayerController.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

namespace
{
	FButtonStyle MakeAltarTransparentButtonStyle()
	{
		FSlateBrush Empty;
		Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(Empty);
		Style.SetHovered(Empty);
		Style.SetPressed(Empty);
		Style.SetDisabled(Empty);
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(0.0f));
		FVelmaraUITheme::ApplyButtonSounds(Style);
		return Style;
	}
}

UVelmaraBloodAltarMenuWidget::UVelmaraBloodAltarMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

UVelmaraBloodAltarMenuWidget* UVelmaraBloodAltarMenuWidget::OpenBloodAltar(APlayerController* PlayerController, AActor* TargetActor, EVelmaraBloodAltarTab InitialTab)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return nullptr;
	}

	UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::GetOrCreate(PlayerController);
	if (!Layout)
	{
		return nullptr;
	}

	if (UVelmaraBloodAltarMenuWidget* Existing = GetActiveAltar(PlayerController))
	{
		Existing->BindTargetActor(TargetActor, InitialTab);
		return Existing;
	}

	return Layout->PushMenuWidget<UVelmaraBloodAltarMenuWidget>(
		UVelmaraBloodAltarMenuWidget::StaticClass(),
		[TargetActor, InitialTab](UVelmaraBloodAltarMenuWidget& Altar)
		{
			Altar.BindTargetActor(TargetActor, InitialTab);
		});
}

UVelmaraBloodAltarMenuWidget* UVelmaraBloodAltarMenuWidget::GetActiveAltar(const UObject* WorldContext)
{
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(WorldContext))
	{
		if (UCommonActivatableWidgetStack* Stack = Layout->GetMenuStack())
		{
			return Cast<UVelmaraBloodAltarMenuWidget>(Stack->GetActiveWidget());
		}
	}
	return nullptr;
}

bool UVelmaraBloodAltarMenuWidget::IsActiveForComponent(const UUpgradeComponentBase* Component)
{
	if (!Component)
	{
		return false;
	}
	if (UVelmaraBloodAltarMenuWidget* Active = GetActiveAltar(Component))
	{
		return Active->OwnsComponent(Component);
	}
	return false;
}

void UVelmaraBloodAltarMenuWidget::CloseActiveAltar(const UObject* WorldContext)
{
	if (UVelmaraBloodAltarMenuWidget* Active = GetActiveAltar(WorldContext))
	{
		Active->DeactivateWidget();
	}
}

bool UVelmaraBloodAltarMenuWidget::OwnsComponent(const UUpgradeComponentBase* Component) const
{
	return Component && (Component == AttributeTreeComp.Get() || Component == WeaponUpgradeComp.Get());
}

TOptional<FUIInputConfig> UVelmaraBloodAltarMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraBloodAltarMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraBloodAltarMenuWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("AltarRootOverlay"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("AltarBlur"));
	Blur->SetBlurStrength(10.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AltarDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.76f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* LeftGradient = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarLeftGrad"));
	LeftGradient->SetVisibility(ESlateVisibility::HitTestInvisible);
	LeftGradient->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradHFadeLeftToRight),
		FVector2D(64.0f, 64.0f),
		FVelmaraUITheme::Void(0.55f)));
	if (UOverlaySlot* GradSlot = RootOverlay->AddChildToOverlay(LeftGradient))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* LeftSpineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarLeftSpineSize"));
	LeftSpineSize->SetWidthOverride(18.0f);
	UImage* LeftSpine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarLeftSpine"));
	LeftSpine->SetVisibility(ESlateVisibility::HitTestInvisible);
	LeftSpine->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSidebarLineV),
		FVector2D(18.0f, 760.0f),
		FVelmaraUITheme::BloodBright(0.65f)));
	LeftSpineSize->SetContent(LeftSpine);
	if (UOverlaySlot* SpineSlot = RootOverlay->AddChildToOverlay(LeftSpineSize))
	{
		SpineSlot->SetHorizontalAlignment(HAlign_Left);
		SpineSlot->SetVerticalAlignment(VAlign_Fill);
		SpineSlot->SetPadding(FMargin(36.0f, 92.0f, 0.0f, 92.0f));
	}

	UVerticalBox* MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AltarMainStack"));
	if (UOverlaySlot* MainSlot = RootOverlay->AddChildToOverlay(MainStack))
	{
		MainSlot->SetHorizontalAlignment(HAlign_Fill);
		MainSlot->SetVerticalAlignment(VAlign_Fill);
		MainSlot->SetPadding(FMargin(64.0f, 34.0f, 64.0f, 34.0f));
	}

	UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AltarTopBar"));
	if (UVerticalBoxSlot* TopSlot = MainStack->AddChildToVerticalBox(TopBar))
	{
		TopSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	UVerticalBox* BrandCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AltarBrandCol"));
	UCommonTextBlock* BrandMain = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarBrandMain"));
	BrandMain->SetText(NSLOCTEXT("VelmaraUI", "AltarBrand", "V E L M A R A"));
	BrandMain->SetFont(FVelmaraUITheme::GetDisplayFont(22, 260));
	BrandMain->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone(0.75f)));
	BrandCol->AddChildToVerticalBox(BrandMain);

	UCommonTextBlock* BrandSub = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarBrandSub"));
	BrandSub->SetText(NSLOCTEXT("VelmaraUI", "AltarSub", "ALTAR DE SANGRE"));
	BrandSub->SetFont(FVelmaraUITheme::GetDisplayFont(11, 220));
	BrandSub->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright(0.85f)));
	BrandCol->AddChildToVerticalBox(BrandSub)->SetPadding(FMargin(2.0f, 2.0f, 0.0f, 0.0f));

	if (UHorizontalBoxSlot* BrandSlot = TopBar->AddChildToHorizontalBox(BrandCol))
	{
		BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BrandSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* TabsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AltarTabsRow"));

	auto BuildTopTab = [&](const TCHAR* BaseName, const FText& Label, TObjectPtr<UButton>& OutButton, TObjectPtr<UCommonTextBlock>& OutText, TObjectPtr<UImage>& OutLine, void (UVelmaraBloodAltarMenuWidget::*Handler)())
	{
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*FString::Printf(TEXT("%sBtn"), BaseName)));
		OutButton->SetStyle(MakeAltarTransparentButtonStyle());
		OutButton->OnClicked.AddDynamic(this, Handler);

		UVerticalBox* TabBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		OutText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass());
		OutText->SetText(Label);
		OutText->SetFont(FVelmaraUITheme::GetDisplayFont(24, 220));
		OutText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		if (UVerticalBoxSlot* TSlot = TabBox->AddChildToVerticalBox(OutText))
		{
			TSlot->SetHorizontalAlignment(HAlign_Center);
			TSlot->SetPadding(FMargin(16.0f, 4.0f, 16.0f, 6.0f));
		}

		USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LineSize->SetWidthOverride(190.0f);
		LineSize->SetHeightOverride(12.0f);
		OutLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		OutLine->SetBrush(FVelmaraUITheme::MakeImageBrush(
			FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnTabActiveLine),
			FVector2D(190.0f, 12.0f),
			FVelmaraUITheme::BloodBright()));
		OutLine->SetVisibility(ESlateVisibility::Hidden);
		LineSize->SetContent(OutLine);
		if (UVerticalBoxSlot* LSlot = TabBox->AddChildToVerticalBox(LineSize))
		{
			LSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (UButtonSlot* BSlot = Cast<UButtonSlot>(OutButton->SetContent(TabBox)))
		{
			BSlot->SetHorizontalAlignment(HAlign_Center);
			BSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UHorizontalBoxSlot* RowSlot = TabsRow->AddChildToHorizontalBox(OutButton))
		{
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
	};

	BuildTopTab(TEXT("AltarTabAttr"), NSLOCTEXT("VelmaraUI", "AltarTabAttributes", "ATRIBUTOS"), AttributesTabButton, AttributesTabText, AttributesTabLine, &UVelmaraBloodAltarMenuWidget::HandleAttributesTabClicked);

	USizeBox* TabStarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarTabStarSize"));
	TabStarSize->SetWidthOverride(18.0f);
	TabStarSize->SetHeightOverride(18.0f);
	UImage* TabStarImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarTabStarImg"));
	TabStarImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnRowStar),
		FVector2D(18.0f, 18.0f),
		FVelmaraUITheme::Bone(0.85f)));
	TabStarSize->SetContent(TabStarImg);
	if (UHorizontalBoxSlot* StarSlot = TabsRow->AddChildToHorizontalBox(TabStarSize))
	{
		StarSlot->SetVerticalAlignment(VAlign_Center);
		StarSlot->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 14.0f));
	}

	BuildTopTab(TEXT("AltarTabWeap"), NSLOCTEXT("VelmaraUI", "AltarTabWeapons", "MEJORAS DE ARMAS"), WeaponsTabButton, WeaponsTabText, WeaponsTabLine, &UVelmaraBloodAltarMenuWidget::HandleWeaponsTabClicked);

	if (UHorizontalBoxSlot* CenterTabsSlot = TopBar->AddChildToHorizontalBox(TabsRow))
	{
		CenterTabsSlot->SetHorizontalAlignment(HAlign_Center);
		CenterTabsSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* RightCurrencyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AltarCurrencyBox"));

	USpacer* RightPush = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UHorizontalBoxSlot* PushSlot = RightCurrencyBox->AddChildToHorizontalBox(RightPush))
	{
		PushSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	USizeBox* BloodDropSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarBloodIconSize"));
	BloodDropSize->SetWidthOverride(22.0f);
	BloodDropSize->SetHeightOverride(22.0f);
	UImage* BloodDropImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarBloodIconImg"));
	BloodDropImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::IconStatBlood),
		FVector2D(22.0f, 22.0f),
		FVelmaraUITheme::Bone()));
	BloodDropSize->SetContent(BloodDropImg);
	if (UHorizontalBoxSlot* DropSlot = RightCurrencyBox->AddChildToHorizontalBox(BloodDropSize))
	{
		DropSlot->SetVerticalAlignment(VAlign_Center);
		DropSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	CurrencyValueText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarCurrencyText"));
	CurrencyValueText->SetText(NSLOCTEXT("VelmaraUI", "AltarCurrencyZero", "0 SANGRE"));
	CurrencyValueText->SetFont(FVelmaraUITheme::GetDisplayFont(22, 160));
	CurrencyValueText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UHorizontalBoxSlot* ValSlot = RightCurrencyBox->AddChildToHorizontalBox(CurrencyValueText))
	{
		ValSlot->SetVerticalAlignment(VAlign_Center);
		ValSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
	}

	USizeBox* CrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarCrestSize"));
	CrestSize->SetWidthOverride(32.0f);
	CrestSize->SetHeightOverride(76.0f);
	UImage* CrestImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarCrestImg"));
	CrestImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(32.0f, 76.0f),
		FVelmaraUITheme::BloodBright(0.92f)));
	CrestSize->SetContent(CrestImg);
	if (UHorizontalBoxSlot* CrestSlot = RightCurrencyBox->AddChildToHorizontalBox(CrestSize))
	{
		CrestSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UHorizontalBoxSlot* RightSlot = TopBar->AddChildToHorizontalBox(RightCurrencyBox))
	{
		RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RightSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* BodyColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AltarBodyColumns"));
	if (UVerticalBoxSlot* BodySlot = MainStack->AddChildToVerticalBox(BodyColumns))
	{
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BodySlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 18.0f));
	}

	ContentSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("AltarContentSwitcher"));

	AttributeTreeWidget = CreateWidget<UAttributeTreeMenuWidget>(GetOwningPlayer(), UAttributeTreeMenuWidget::StaticClass());
	if (AttributeTreeWidget)
	{
		AttributeTreeWidget->SetEmbeddedMode(true);
		AttributeTreeWidget->OnRowSelected.AddUObject(this, &UVelmaraBloodAltarMenuWidget::HandleAttributeRowSelected);
		AttributeTreeWidget->OnFeedbackMessage.AddUObject(this, &UVelmaraBloodAltarMenuWidget::HandleChildFeedback);
		ContentSwitcher->AddChild(AttributeTreeWidget);
	}

	WeaponUpgradeWidget = CreateWidget<UWeaponUpgradeMenuWidget>(GetOwningPlayer(), UWeaponUpgradeMenuWidget::StaticClass());
	if (WeaponUpgradeWidget)
	{
		WeaponUpgradeWidget->SetEmbeddedMode(true);
		WeaponUpgradeWidget->OnComboSelected.AddUObject(this, &UVelmaraBloodAltarMenuWidget::HandleWeaponComboSelected);
		WeaponUpgradeWidget->OnFeedbackMessage.AddUObject(this, &UVelmaraBloodAltarMenuWidget::HandleChildFeedback);
		ContentSwitcher->AddChild(WeaponUpgradeWidget);
	}

	if (UHorizontalBoxSlot* SwitchSlot = BodyColumns->AddChildToHorizontalBox(ContentSwitcher))
	{
		SwitchSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SwitchSlot->SetHorizontalAlignment(HAlign_Fill);
		SwitchSlot->SetVerticalAlignment(VAlign_Fill);
		SwitchSlot->SetPadding(FMargin(0.0f, 0.0f, 40.0f, 0.0f));
	}

	USizeBox* RightPanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarDetailsSize"));
	RightPanelSize->SetWidthOverride(460.0f);
	DetailsPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("AltarDetailsPanel"));
	DetailsPanel->SetUseMinimalBrackets(true);
	DetailsPanel->SetBackdropAlpha(0.78f);
	DetailsPanel->SetShowSignature(true);
	DetailsPanel->SetShowTopStar(true);
	RightPanelSize->SetContent(DetailsPanel);

	if (UHorizontalBoxSlot* RightColSlot = BodyColumns->AddChildToHorizontalBox(RightPanelSize))
	{
		RightColSlot->SetHorizontalAlignment(HAlign_Right);
		RightColSlot->SetVerticalAlignment(VAlign_Fill);
		RightColSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 6.0f));
	}

	UVerticalBox* DetailsContent = DetailsPanel->GetContentBox();

	DetailCategoryText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailCat"));
	DetailCategoryText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 220));
	DetailCategoryText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* CatSlot = DetailsContent->AddChildToVerticalBox(DetailCategoryText))
	{
		CatSlot->SetHorizontalAlignment(HAlign_Center);
		CatSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	DetailTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailTitle"));
	DetailTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(24, 180));
	DetailTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	DetailTitleText->SetJustification(ETextJustify::Center);
	DetailTitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* TitleSlot = DetailsContent->AddChildToVerticalBox(DetailTitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* DetSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AltarDetSepSize"));
	DetSepSize->SetHeightOverride(16.0f);
	UImage* DetSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AltarDetSep"));
	DetSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(340.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.8f)));
	DetSepSize->SetContent(DetSep);
	if (UVerticalBoxSlot* SepSlot = DetailsContent->AddChildToVerticalBox(DetSepSize))
	{
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	DetailBodyText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailBody"));
	DetailBodyText->SetFont(FVelmaraUITheme::GetBodyFont(17, 0));
	DetailBodyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone(0.90f)));
	DetailBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodyTxtSlot = DetailsContent->AddChildToVerticalBox(DetailBodyText))
	{
		BodyTxtSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	DetailMetaText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailMeta"));
	DetailMetaText->SetFont(FVelmaraUITheme::GetBoldBodyFont(15, 0));
	DetailMetaText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Spectral()));
	DetailMetaText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* MetaSlot = DetailsContent->AddChildToVerticalBox(DetailMetaText))
	{
		MetaSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailCostText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailCost"));
	DetailCostText->SetFont(FVelmaraUITheme::GetBodyFont(16, 0));
	DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	DetailCostText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* CostSlot = DetailsContent->AddChildToVerticalBox(DetailCostText))
	{
		CostSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailFeedbackText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarDetailFeedback"));
	DetailFeedbackText->SetFont(FVelmaraUITheme::GetBoldBodyFont(15, 0));
	DetailFeedbackText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	DetailFeedbackText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* FbSlot = DetailsContent->AddChildToVerticalBox(DetailFeedbackText))
	{
		FbSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 14.0f));
	}

	USpacer* DetailSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UVerticalBoxSlot* SpSlot = DetailsContent->AddChildToVerticalBox(DetailSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UCommonTextBlock* QuoteText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("AltarQuoteText"));
	QuoteText->SetText(NSLOCTEXT("VelmaraUI", "AltarQuote", "\"Todo sacrificio deja huella. Sangre forja existencia.\""));
	QuoteText->SetFont(FVelmaraUITheme::GetItalicFont(16, 10));
	QuoteText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.70f)));
	QuoteText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* QSlot = DetailsContent->AddChildToVerticalBox(QuoteText))
	{
		QSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("AltarFooterBar"));
	FooterBar->SetActions({
		FVelmaraFooterAction(TEXT("TabNext"), NSLOCTEXT("VelmaraUI", "AltarFooterTabs", "CAMBIAR PESTAÑA")),
		FVelmaraFooterAction(TEXT("Confirm"), NSLOCTEXT("VelmaraUI", "AltarFooterUpgrade", "MEJORAR")),
		FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "AltarFooterExit", "SALIR DEL ALTAR"))
	});
	MainStack->AddChildToVerticalBox(FooterBar);

	RefreshTabVisuals();
}

void UVelmaraBloodAltarMenuWidget::BindTargetActor(AActor* TargetActor, EVelmaraBloodAltarTab InitialTab)
{
	UnbindCurrencyDelegates();

	AActor* ResolvedActor = TargetActor;
	if (!ResolvedActor)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			ResolvedActor = PC->GetPawn();
			if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC))
			{
				if (CustomPC->GetOriginalBody())
				{
					ResolvedActor = CustomPC->GetOriginalBody();
				}
			}
		}
	}

	AttributeTreeComp = ResolvedActor ? ResolvedActor->FindComponentByClass<UAttributeTreeComponent>() : nullptr;
	WeaponUpgradeComp = ResolvedActor ? ResolvedActor->FindComponentByClass<UWeaponUpgradeComponent>() : nullptr;
	ProgressionComp = ResolvedActor ? ResolvedActor->FindComponentByClass<UProgressionComponent>() : nullptr;

	if (!AttributeTreeComp.IsValid() && !WeaponUpgradeComp.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC))
			{
				if (AActor* Original = CustomPC->GetOriginalBody())
				{
					AttributeTreeComp = Original->FindComponentByClass<UAttributeTreeComponent>();
					WeaponUpgradeComp = Original->FindComponentByClass<UWeaponUpgradeComponent>();
					ProgressionComp = Original->FindComponentByClass<UProgressionComponent>();
				}
			}
		}
	}

	if (AttributeTreeWidget && AttributeTreeComp.IsValid())
	{
		AttributeTreeWidget->SetUpgradeComponent(AttributeTreeComp.Get());
	}

	if (WeaponUpgradeWidget && WeaponUpgradeComp.IsValid())
	{
		WeaponUpgradeWidget->SetUpgradeComponent(WeaponUpgradeComp.Get());
	}

	BindCurrencyDelegates();
	RefreshCurrencyHeader();

	EVelmaraBloodAltarTab ChosenTab = InitialTab;
	if (ChosenTab == EVelmaraBloodAltarTab::Attributes && !AttributeTreeComp.IsValid() && WeaponUpgradeComp.IsValid())
	{
		ChosenTab = EVelmaraBloodAltarTab::WeaponUpgrades;
	}
	else if (ChosenTab == EVelmaraBloodAltarTab::WeaponUpgrades && !WeaponUpgradeComp.IsValid() && AttributeTreeComp.IsValid())
	{
		ChosenTab = EVelmaraBloodAltarTab::Attributes;
	}

	SetActiveTab(ChosenTab);
}

void UVelmaraBloodAltarMenuWidget::BindCurrencyDelegates()
{
	if (UAttributeTreeComponent* Attr = AttributeTreeComp.Get())
	{
		Attr->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
	if (UWeaponUpgradeComponent* Weap = WeaponUpgradeComp.Get())
	{
		Weap->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.AddUniqueDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
}

void UVelmaraBloodAltarMenuWidget::UnbindCurrencyDelegates()
{
	if (UAttributeTreeComponent* Attr = AttributeTreeComp.Get())
	{
		Attr->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
	if (UWeaponUpgradeComponent* Weap = WeaponUpgradeComp.Get())
	{
		Weap->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
	if (UProgressionComponent* Prog = ProgressionComp.Get())
	{
		Prog->OnCurrencyChanged.RemoveDynamic(this, &UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged);
	}
}

int32 UVelmaraBloodAltarMenuWidget::ResolveBloodAmount() const
{
	if (const UAttributeTreeComponent* Attr = AttributeTreeComp.Get())
	{
		if (Attr->HasCurrencyProvider())
		{
			return Attr->GetCurrencyAmount();
		}
	}
	if (const UWeaponUpgradeComponent* Weap = WeaponUpgradeComp.Get())
	{
		if (Weap->HasCurrencyProvider())
		{
			return Weap->GetCurrencyAmount();
		}
	}
	if (const UProgressionComponent* Prog = ProgressionComp.Get())
	{
		return Prog->GetAvailableCurrency();
	}
	return 0;
}

void UVelmaraBloodAltarMenuWidget::RefreshCurrencyHeader()
{
	if (!CurrencyValueText) return;

	const int32 Amount = ResolveBloodAmount();
	CurrencyValueText->SetText(FText::Format(
		NSLOCTEXT("VelmaraUI", "AltarCurrencyFmt", "{0} SANGRE"),
		FVelmaraUITheme::FormatGroupedNumber(Amount)));
}

void UVelmaraBloodAltarMenuWidget::SetActiveTab(EVelmaraBloodAltarTab NewTab)
{
	ActiveTab = NewTab;

	if (DetailFeedbackText)
	{
		DetailFeedbackText->SetText(FText::GetEmpty());
	}

	if (ContentSwitcher)
	{
		const int32 TargetIdx = (ActiveTab == EVelmaraBloodAltarTab::Attributes) ? 0 : 1;
		if (TargetIdx < ContentSwitcher->GetNumWidgets())
		{
			ContentSwitcher->SetActiveWidgetIndex(TargetIdx);
		}
	}

	RefreshTabVisuals();

	if (ActiveTab == EVelmaraBloodAltarTab::Attributes && AttributeTreeWidget)
	{
		AttributeTreeWidget->RefreshMenu();
		if (UWidget* Target = AttributeTreeWidget->GetFirstFocusableWidget())
		{
			Target->SetKeyboardFocus();
		}
	}
	else if (ActiveTab == EVelmaraBloodAltarTab::WeaponUpgrades && WeaponUpgradeWidget)
	{
		WeaponUpgradeWidget->RefreshMenu();
		if (UWidget* Target = WeaponUpgradeWidget->GetFirstFocusableWidget())
		{
			Target->SetKeyboardFocus();
		}
	}
}

void UVelmaraBloodAltarMenuWidget::RefreshTabVisuals()
{
	const bool bAttrActive = ActiveTab == EVelmaraBloodAltarTab::Attributes;

	if (AttributesTabText)
	{
		AttributesTabText->SetColorAndOpacity(FSlateColor(bAttrActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted(0.75f)));
	}
	if (AttributesTabLine)
	{
		AttributesTabLine->SetVisibility(bAttrActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}

	if (WeaponsTabText)
	{
		WeaponsTabText->SetColorAndOpacity(FSlateColor(!bAttrActive ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::BoneMuted(0.75f)));
	}
	if (WeaponsTabLine)
	{
		WeaponsTabLine->SetVisibility(!bAttrActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UVelmaraBloodAltarMenuWidget::UpdateDetailsForAttribute(FGameplayTag UpgradeTag)
{
	const UAttributeTreeComponent* Tree = AttributeTreeComp.Get();
	FAttributeUpgradeDefinition Def;
	if (!Tree || !Tree->GetUpgradeDefinition(UpgradeTag, Def))
	{
		if (DetailCategoryText) DetailCategoryText->SetText(NSLOCTEXT("VelmaraUI", "AltarDetailCatAttr", "ATRIBUTO DE SANGRE"));
		if (DetailTitleText) DetailTitleText->SetText(NSLOCTEXT("VelmaraUI", "AltarDetailNone", "ALTAR DE SANGRE"));
		if (DetailBodyText) DetailBodyText->SetText(NSLOCTEXT("VelmaraUI", "AltarDetailNoneBody", "Selecciona un atributo para ofrendar tu Sangre."));
		if (DetailMetaText) DetailMetaText->SetText(FText::GetEmpty());
		if (DetailCostText) DetailCostText->SetText(FText::GetEmpty());
		return;
	}

	const int32 CurrentLevel = Tree->GetUpgradeLevel(UpgradeTag);
	const int32 MaxLevel = Def.GetMaxLevel();
	const bool bIsMax = CurrentLevel >= MaxLevel;
	const float CurrentVal = Tree->GetCurrentAttributeValue(UpgradeTag);
	const FText CurBonus = AttributeTreeUtils::FormatBonus(Def.GetTotalBonusAtLevel(CurrentLevel), Def.DisplayFormat);

	if (DetailCategoryText)
	{
		DetailCategoryText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "AltarAttrLevelFmt", "ATRIBUTO  ·  NIVEL {0} / {1}"),
			FText::FromString(AttributeTreeUtils::ToRoman(CurrentLevel)),
			FText::FromString(AttributeTreeUtils::ToRoman(MaxLevel))));
	}

	if (DetailTitleText)
	{
		DetailTitleText->SetText(Def.DisplayName.ToUpper());
	}

	if (DetailBodyText)
	{
		DetailBodyText->SetText(Def.Description);
	}

	if (DetailMetaText)
	{
		const FText CurValText = AttributeTreeUtils::FormatValue(CurrentVal, Def.DisplayFormat);
		if (bIsMax)
		{
			DetailMetaText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarAttrMetaMax", "Valor actual: {0}  ·  Bonificación total: {1} (MÁXIMO)"),
				CurValText, CurBonus));
		}
		else
		{
			const FText NextBonus = AttributeTreeUtils::FormatBonus(Def.GetTotalBonusAtLevel(CurrentLevel + 1), Def.DisplayFormat);
			DetailMetaText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarAttrMetaNext", "Valor actual: {0}  ·  Bonificación: {1}  →  {2}"),
				CurValText, CurBonus, NextBonus));
		}
	}

	if (DetailCostText)
	{
		if (bIsMax)
		{
			DetailCostText->SetText(NSLOCTEXT("VelmaraUI", "AltarAttrMaxCost", "Esta rama ha alcanzado su plenitud."));
			DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		}
		else if (Tree->AreUpgradesFree())
		{
			DetailCostText->SetText(NSLOCTEXT("VelmaraUI", "AltarAttrFreeCost", "Ofrenda: Gratis (modo prueba)"));
			DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		}
		else
		{
			const int32 Cost = Tree->GetNextLevelCost(UpgradeTag);
			const int32 Available = ResolveBloodAmount();
			const bool bAfford = Tree->CanAffordUpgrade(UpgradeTag);
			DetailCostText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarAttrCostFmt", "Costo Nivel {0}: {1} Sangre  ·  Disponible: {2}"),
				FText::FromString(AttributeTreeUtils::ToRoman(CurrentLevel + 1)),
				FVelmaraUITheme::FormatGroupedNumber(Cost),
				FVelmaraUITheme::FormatGroupedNumber(Available)));
			DetailCostText->SetColorAndOpacity(FSlateColor(bAfford ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
		}
	}
}

void UVelmaraBloodAltarMenuWidget::UpdateDetailsForCombo(UWeaponData* Weapon, FGameplayTag UnlockTag)
{
	const UWeaponUpgradeComponent* Upgrades = WeaponUpgradeComp.Get();
	FWeaponComboUpgrade Upgrade;
	if (!Upgrades || !Weapon || !Upgrades->GetComboUpgrade(Weapon, UnlockTag, Upgrade))
	{
		if (DetailCategoryText) DetailCategoryText->SetText(NSLOCTEXT("VelmaraUI", "AltarDetailCatWeap", "MEJORAS DE ARMAS"));
		if (DetailTitleText) DetailTitleText->SetText(Weapon ? WeaponUpgradeUtils::GetWeaponDisplayName(Weapon).ToUpper() : NSLOCTEXT("VelmaraUI", "AltarNoWeaponTitle", "SIN ARMA"));
		if (DetailBodyText) DetailBodyText->SetText(NSLOCTEXT("VelmaraUI", "AltarComboEmptyBody", "Selecciona un combo para despertar nuevas técnicas de combate."));
		if (DetailMetaText) DetailMetaText->SetText(FText::GetEmpty());
		if (DetailCostText) DetailCostText->SetText(FText::GetEmpty());
		return;
	}

	const EWeaponComboState State = Upgrades->GetComboState(Weapon, UnlockTag);
	const FComboInputPath Path = WeaponUpgradeUtils::FindComboPath(Weapon, UnlockTag);

	if (DetailCategoryText)
	{
		DetailCategoryText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "AltarComboCatFmt", "{0}  ·  TÉCNICA DE COMBATE"),
			WeaponUpgradeUtils::GetWeaponDisplayName(Weapon).ToUpper()));
	}

	if (DetailTitleText)
	{
		DetailTitleText->SetText(Upgrade.DisplayName.ToUpper());
	}

	if (DetailBodyText)
	{
		DetailBodyText->SetText(Upgrade.Description.IsEmpty()
			? NSLOCTEXT("VelmaraUI", "AltarComboFallbackDesc", "Desbloquea esta secuencia de ataque para expandir el repertorio del arma.")
			: Upgrade.Description);
	}

	if (DetailMetaText)
	{
		FText InputLine = WeaponUpgradeUtils::DescribeComboInput(Path);
		const FGameplayTagContainer Missing = Upgrades->GetMissingRequirements(Weapon, UnlockTag);
		if (!Missing.IsEmpty())
		{
			TArray<FText> ReqNames;
			for (const FGameplayTag& Req : Missing)
			{
				FWeaponComboUpgrade ReqUpgrade;
				ReqNames.Add(Upgrades->GetComboUpgrade(Weapon, Req, ReqUpgrade)
					? ReqUpgrade.DisplayName
					: FText::FromName(Req.GetTagName()));
			}
			DetailMetaText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarComboMetaReq", "Secuencia: {0}\nRequiere: {1}"),
				InputLine,
				FText::Join(FText::FromString(TEXT(", ")), ReqNames)));
		}
		else
		{
			DetailMetaText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarComboMetaFmt", "Secuencia: {0}"),
				InputLine));
		}
	}

	if (DetailCostText)
	{
		if (State == EWeaponComboState::Unlocked)
		{
			DetailCostText->SetText(NSLOCTEXT("VelmaraUI", "AltarComboUnlockedCost", "Técnica ya consagrada."));
			DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
		}
		else if (Upgrades->AreUpgradesFree())
		{
			DetailCostText->SetText(NSLOCTEXT("VelmaraUI", "AltarComboFreeCost", "Ofrenda: Gratis (modo prueba)"));
			DetailCostText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
		}
		else
		{
			const bool bAfford = Upgrades->CanAffordCombo(Weapon, UnlockTag);
			DetailCostText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "AltarComboCostFmt", "Ofrenda: {0} Sangre  ·  Disponible: {1}"),
				FVelmaraUITheme::FormatGroupedNumber(Upgrade.Cost),
				FVelmaraUITheme::FormatGroupedNumber(ResolveBloodAmount())));
			DetailCostText->SetColorAndOpacity(FSlateColor(
				(State == EWeaponComboState::Available && bAfford) ? FVelmaraUITheme::Bone() : FVelmaraUITheme::BloodBright()));
		}
	}
}

void UVelmaraBloodAltarMenuWidget::ShowFeedback(const FText& Message, bool bIsError)
{
	if (DetailFeedbackText)
	{
		DetailFeedbackText->SetText(Message);
		DetailFeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Bone()));
	}
	RefreshCurrencyHeader();
}

void UVelmaraBloodAltarMenuWidget::ApplyPauseState(bool bPaused)
{
	UGameplayStatics::SetGamePaused(GetWorld(), bPaused);
	if (UVelmaraGameInstance* GI = Cast<UVelmaraGameInstance>(GetGameInstance()))
	{
		GI->SetPauseAudioMixActive(bPaused);
	}
	if (ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(GetOwningPlayer()))
	{
		CustomPC->SetPauseAudioMixState(bPaused);
	}
}

void UVelmaraBloodAltarMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	ApplyPauseState(true);
	RefreshCurrencyHeader();

	if (UAttributeTreeComponent* Attr = AttributeTreeComp.Get())
	{
		Attr->OnMenuToggled.Broadcast(true);
	}
	if (UWeaponUpgradeComponent* Weap = WeaponUpgradeComp.Get())
	{
		Weap->OnMenuToggled.Broadcast(true);
	}

	if (UWidget* FocusTarget = NativeGetDesiredFocusTarget())
	{
		FocusTarget->SetKeyboardFocus();
	}
}

void UVelmaraBloodAltarMenuWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	UnbindCurrencyDelegates();

	if (UAttributeTreeComponent* Attr = AttributeTreeComp.Get())
	{
		Attr->OnMenuToggled.Broadcast(false);
	}
	if (UWeaponUpgradeComponent* Weap = WeaponUpgradeComp.Get())
	{
		Weap->OnMenuToggled.Broadcast(false);
	}

	UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this);
	if (!Layout || !Layout->HasActiveMenuOrModal())
	{
		ApplyPauseState(false);
	}
}

UWidget* UVelmaraBloodAltarMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (ActiveTab == EVelmaraBloodAltarTab::Attributes && AttributeTreeWidget)
	{
		if (UWidget* Target = AttributeTreeWidget->GetFirstFocusableWidget())
		{
			return Target;
		}
	}
	if (ActiveTab == EVelmaraBloodAltarTab::WeaponUpgrades && WeaponUpgradeWidget)
	{
		if (UWidget* Target = WeaponUpgradeWidget->GetFirstFocusableWidget())
		{
			return Target;
		}
	}
	return AttributesTabButton;
}

bool UVelmaraBloodAltarMenuWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

FReply UVelmaraBloodAltarMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		DeactivateWidget();
		return FReply::Handled();
	}

	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		const EVelmaraBloodAltarTab NextTab = (ActiveTab == EVelmaraBloodAltarTab::Attributes)
			? EVelmaraBloodAltarTab::WeaponUpgrades
			: EVelmaraBloodAltarTab::Attributes;
		SetActiveTab(NextTab);
		return FReply::Handled();
	}

	if (ActiveTab == EVelmaraBloodAltarTab::WeaponUpgrades && WeaponUpgradeWidget)
	{
		if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
		{
			WeaponUpgradeWidget->CycleWeapon(-1);
			return FReply::Handled();
		}
		if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
		{
			WeaponUpgradeWidget->CycleWeapon(1);
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraBloodAltarMenuWidget::HandleAttributesTabClicked()
{
	SetActiveTab(EVelmaraBloodAltarTab::Attributes);
}

void UVelmaraBloodAltarMenuWidget::HandleWeaponsTabClicked()
{
	SetActiveTab(EVelmaraBloodAltarTab::WeaponUpgrades);
}

void UVelmaraBloodAltarMenuWidget::HandleCurrencyChanged(int32 NewAmount)
{
	RefreshCurrencyHeader();
}

void UVelmaraBloodAltarMenuWidget::HandleAttributeRowSelected(FGameplayTag UpgradeTag)
{
	if (ActiveTab == EVelmaraBloodAltarTab::Attributes)
	{
		UpdateDetailsForAttribute(UpgradeTag);
	}
}

void UVelmaraBloodAltarMenuWidget::HandleWeaponComboSelected(UWeaponData* Weapon, FGameplayTag UnlockTag)
{
	if (ActiveTab == EVelmaraBloodAltarTab::WeaponUpgrades)
	{
		UpdateDetailsForCombo(Weapon, UnlockTag);
	}
}

void UVelmaraBloodAltarMenuWidget::HandleChildFeedback(const FText& Message, bool bIsError)
{
	ShowFeedback(Message, bIsError);
}
