#include "UI/Widgets/Menus/VelmaraSaveSlotsWidget.h"
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
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Features/SaveSystem/Core/PlayerProgressSaveGame.h"
#include "Features/SaveSystem/Subsystems/SaveGameSubsystem.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UI/Widgets/Common/VelmaraButtonBase.h"
#include "UI/Widgets/Common/VelmaraConfirmModalWidget.h"
#include "UI/Widgets/Common/VelmaraFooterBar.h"

UVelmaraSaveSlotsWidget::UVelmaraSaveSlotsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bSupportsActivationFocus = true;
	bAutoActivate = false;
}

TOptional<FUIInputConfig> UVelmaraSaveSlotsWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
}

TSharedRef<SWidget> UVelmaraSaveSlotsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreenTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraSaveSlotsWidget::BuildScreenTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SaveSlotsRoot"));
	WidgetTree->RootWidget = RootOverlay;

	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("SaveSlotsBlur"));
	Blur->SetBlurStrength(10.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotsDimBg"));
	DimBg->SetBrush(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Void(0.82f)));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(DimBg))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* VignetteImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SaveSlotsVignette"));
	VignetteImg->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	VignetteImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradVignetteRadial),
		FVector2D(512.0f, 512.0f),
		FVelmaraUITheme::Void(0.85f)));
	if (UOverlaySlot* VigSlot = RootOverlay->AddChildToOverlay(VignetteImg))
	{
		VigSlot->SetHorizontalAlignment(HAlign_Fill);
		VigSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* TopCrestSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlotsCrestSize"));
	TopCrestSize->SetWidthOverride(34.0f);
	TopCrestSize->SetHeightOverride(96.0f);
	UImage* TopCrest = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SaveSlotsCrest"));
	TopCrest->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnCrestVelmara),
		FVector2D(34.0f, 96.0f),
		FVelmaraUITheme::BloodBright(0.90f)));
	TopCrestSize->SetContent(TopCrest);
	if (UOverlaySlot* CrestSlot = RootOverlay->AddChildToOverlay(TopCrestSize))
	{
		CrestSlot->SetHorizontalAlignment(HAlign_Right);
		CrestSlot->SetVerticalAlignment(VAlign_Top);
		CrestSlot->SetPadding(FMargin(0.0f, 44.0f, 64.0f, 0.0f));
	}

	UVerticalBox* PageVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotsPageVBox"));
	if (UOverlaySlot* PageSlot = RootOverlay->AddChildToOverlay(PageVBox))
	{
		PageSlot->SetHorizontalAlignment(HAlign_Fill);
		PageSlot->SetVerticalAlignment(VAlign_Fill);
		PageSlot->SetPadding(FMargin(72.0f, 52.0f, 72.0f, 44.0f));
	}

	ModeSubtitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotsModeSubtitle"));
	ModeSubtitleText->SetFont(FVelmaraUITheme::GetDisplayFont(15, 240));
	ModeSubtitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UVerticalBoxSlot* SubSlot = PageVBox->AddChildToVerticalBox(ModeSubtitleText))
	{
		SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	HeaderTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotsHeaderTitle"));
	HeaderTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(42, 240));
	HeaderTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* HeadSlot = PageVBox->AddChildToVerticalBox(HeaderTitleText))
	{
		HeadSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotsSepSize"));
	SepSize->SetWidthOverride(520.0f);
	SepSize->SetHeightOverride(28.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotsSepImg"));
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepStar),
		FVector2D(520.0f, 28.0f),
		FVelmaraUITheme::Bone(0.85f)));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = PageVBox->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetHorizontalAlignment(HAlign_Left);
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
	}

	UGridPanel* ColumnsGrid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("SlotsColumnsGrid"));
	ColumnsGrid->SetColumnFill(0, 0.65f);
	ColumnsGrid->SetColumnFill(1, 0.35f);
	if (UVerticalBoxSlot* GridSlot = PageVBox->AddChildToVerticalBox(ColumnsGrid))
	{
		GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}

	SlotsContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotsCardsVBox"));
	if (UGridSlot* LeftCol = ColumnsGrid->AddChildToGrid(SlotsContainer, 0, 0))
	{
		LeftCol->SetHorizontalAlignment(HAlign_Fill);
		LeftCol->SetVerticalAlignment(VAlign_Top);
		LeftCol->SetPadding(FMargin(0.0f, 0.0f, 44.0f, 0.0f));
	}

	SlotCards.Reset();
	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		UVelmaraSaveSlotCardWidget* Card = WidgetTree->ConstructWidget<UVelmaraSaveSlotCardWidget>(UVelmaraSaveSlotCardWidget::StaticClass());
		Card->OnSlotActivated.AddUObject(this, &UVelmaraSaveSlotsWidget::HandleSlotActivated);
		Card->OnSlotHighlighted.AddUObject(this, &UVelmaraSaveSlotsWidget::HandleSlotHighlighted);
		Card->OnSlotDeleteRequested.AddUObject(this, &UVelmaraSaveSlotsWidget::HandleSlotDeleteRequested);
		if (UVerticalBoxSlot* CardSlot = SlotsContainer->AddChildToVerticalBox(Card))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Fill);
			CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}
		SlotCards.Add(Card);
	}

	DetailsPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("SlotDetailsPanel"));
	DetailsPanel->SetUseMinimalBrackets(false);
	DetailsPanel->SetBackdropAlpha(0.78f);
	DetailsPanel->SetShowSignature(true);
	if (UGridSlot* RightCol = ColumnsGrid->AddChildToGrid(DetailsPanel, 0, 1))
	{
		RightCol->SetHorizontalAlignment(HAlign_Fill);
		RightCol->SetVerticalAlignment(VAlign_Fill);
		RightCol->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	UVerticalBox* DetailsContent = DetailsPanel->GetContentBox();

	DetailTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotDetailTitle"));
	DetailTitleText->SetFont(FVelmaraUITheme::GetDisplayFont(23, 200));
	DetailTitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	if (UVerticalBoxSlot* DT = DetailsContent->AddChildToVerticalBox(DetailTitleText))
	{
		DT->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	USizeBox* DetailSepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotDetailSepSize"));
	DetailSepSize->SetHeightOverride(16.0f);
	UImage* DetailSep = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SlotDetailSep"));
	DetailSep->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(320.0f, 16.0f),
		FVelmaraUITheme::BoneMuted(0.75f)));
	DetailSepSize->SetContent(DetailSep);
	if (UVerticalBoxSlot* DS = DetailsContent->AddChildToVerticalBox(DetailSepSize))
	{
		DS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	DetailBodyText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotDetailBody"));
	DetailBodyText->SetFont(FVelmaraUITheme::GetBodyFont(18, 0));
	DetailBodyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted()));
	DetailBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DB = DetailsContent->AddChildToVerticalBox(DetailBodyText))
	{
		DB->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	USpacer* DetailSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("SlotDetailSpacer"));
	if (UVerticalBoxSlot* SpSlot = DetailsContent->AddChildToVerticalBox(DetailSpacer))
	{
		SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	DetailQuoteText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("SlotDetailQuote"));
	DetailQuoteText->SetFont(FVelmaraUITheme::GetItalicFont(17, 10));
	DetailQuoteText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.75f)));
	DetailQuoteText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DQ = DetailsContent->AddChildToVerticalBox(DetailQuoteText))
	{
		DQ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
	}

	UHorizontalBox* ActionsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotPanelActions"));
	if (UVerticalBoxSlot* ActSlot = DetailsContent->AddChildToVerticalBox(ActionsRow))
	{
		ActSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
	}

	DeleteSlotButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("SlotDeleteBtn"));
	DeleteSlotButton->SetVariant(EVelmaraButtonVariant::HexAction);
	DeleteSlotButton->SetFontSize(15);
	DeleteSlotButton->SetButtonMinSize(170.0f, 44.0f);
	DeleteSlotButton->SetButtonText(NSLOCTEXT("VelmaraUI", "BtnDeleteSlot", "BORRAR RANURA"));
	DeleteSlotButton->OnClicked().AddUObject(this, &UVelmaraSaveSlotsWidget::HandleDeleteButtonClicked);
	if (UHorizontalBoxSlot* DelSlot = ActionsRow->AddChildToHorizontalBox(DeleteSlotButton))
	{
		DelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	BackButton = WidgetTree->ConstructWidget<UVelmaraButtonBase>(UVelmaraButtonBase::StaticClass(), TEXT("SlotBackBtn"));
	BackButton->SetVariant(EVelmaraButtonVariant::HexAction);
	BackButton->SetFontSize(15);
	BackButton->SetButtonMinSize(130.0f, 44.0f);
	BackButton->SetButtonText(NSLOCTEXT("VelmaraUI", "BtnBack", "ATRÁS"));
	BackButton->OnClicked().AddUObject(this, &UVelmaraSaveSlotsWidget::HandleBackButtonClicked);
	ActionsRow->AddChildToHorizontalBox(BackButton);

	FooterBar = WidgetTree->ConstructWidget<UVelmaraFooterBar>(UVelmaraFooterBar::StaticClass(), TEXT("SlotsFooterBar"));
	if (UVerticalBoxSlot* FootSlot = PageVBox->AddChildToVerticalBox(FooterBar))
	{
		FootSlot->SetHorizontalAlignment(HAlign_Fill);
		FootSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
	}

	UpdateHeaderText();
	RefreshSlots();
}

void UVelmaraSaveSlotsWidget::SetSlotsMode(ESaveSlotsMode InMode)
{
	CurrentMode = InMode;
	UpdateHeaderText();
	RefreshSlots();
}

void UVelmaraSaveSlotsWidget::UpdateHeaderText()
{
	if (!ModeSubtitleText || !HeaderTitleText)
	{
		return;
	}

	if (CurrentMode == ESaveSlotsMode::NewGame)
	{
		ModeSubtitleText->SetText(NSLOCTEXT("VelmaraUI", "ModeNewGame", "NUEVA PARTIDA"));
		HeaderTitleText->SetText(NSLOCTEXT("VelmaraUI", "SelectSlot", "SELECCIONAR RANURA"));
	}
	else
	{
		ModeSubtitleText->SetText(NSLOCTEXT("VelmaraUI", "ModeLoadGame", "CARGAR PARTIDA"));
		HeaderTitleText->SetText(NSLOCTEXT("VelmaraUI", "SelectSlot", "SELECCIONAR RANURA"));
	}
}

void UVelmaraSaveSlotsWidget::RefreshSlots()
{
	CachedSlotsData.Reset();
	CachedSlotsData.SetNum(3);

	UGameInstance* GI = GetGameInstance();
	UVelmaraGameInstance* VelmaraGI = Cast<UVelmaraGameInstance>(GI);
	USaveGameSubsystem* SaveSubsystem = GI ? GI->GetSubsystem<USaveGameSubsystem>() : nullptr;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FVelmaraSaveSlotViewData& Data = CachedSlotsData[Index];
		Data.SlotIndex = Index;
		Data.bOccupied = false;

		if (SaveSubsystem && SaveSubsystem->DoesSaveGameExist(Index))
		{
			FString LevelName;
			FDateTime Timestamp;
			FString Label;
			if (SaveSubsystem->GetSaveSlotInfo(Index, LevelName, Timestamp, Label))
			{
				Data.bOccupied = true;
				Data.RawLevelName = LevelName;
				Data.Timestamp = Timestamp;
			}
		}

		if (VelmaraGI)
		{
			if (UPlayerProgressSaveGame* Progress = VelmaraGI->GetSaveGameInfo(Index))
			{
				Data.bOccupied = true;
				if (Data.RawLevelName.IsEmpty())
				{
					Data.RawLevelName = Progress->CurrentLevelName;
				}
				if (Data.Timestamp.GetTicks() == 0)
				{
					Data.Timestamp = Progress->Timestamp;
				}
				int32 AttrSum = 0;
				for (const TPair<FGameplayTag, int32>& Pair : Progress->AttributeTreeLevels)
				{
					AttrSum += Pair.Value;
				}
				Data.TotalAttributeLevels = AttrSum;
				Data.UnlockedCombosCount = Progress->UnlockedWeaponCombos.Num();
				Data.SavedActorsCount = Progress->SavedActors.Num();
			}
		}

		if (SlotCards.IsValidIndex(Index) && SlotCards[Index])
		{
			SlotCards[Index]->ConfigureCard(Data, CurrentMode == ESaveSlotsMode::LoadGame);
		}
	}

	UpdateDetailPanel(HighlightedSlotIndex);
	UpdateFooterActions();
}

void UVelmaraSaveSlotsWidget::UpdateDetailPanel(int32 SlotIndex)
{
	if (!CachedSlotsData.IsValidIndex(SlotIndex) || !DetailTitleText || !DetailBodyText || !DetailQuoteText)
	{
		return;
	}

	const FVelmaraSaveSlotViewData& Data = CachedSlotsData[SlotIndex];
	const FString Roman = FVelmaraUITheme::FormatSlotRomanNumeral(SlotIndex);

	if (Data.bOccupied)
	{
		DetailTitleText->SetText(FVelmaraUITheme::FormatLevelDisplayName(Data.RawLevelName));
		const FString DateText = Data.Timestamp.GetTicks() > 0
			? Data.Timestamp.ToString(TEXT("%d/%m/%Y · %H:%M"))
			: TEXT("Sin fecha");
		const FText ActionHint = (CurrentMode == ESaveSlotsMode::NewGame)
			? NSLOCTEXT("VelmaraUI", "DetailOverwriteWarn", "Seleccionar esta ranura solicitará confirmación para sobrescribir la partida actual.")
			: NSLOCTEXT("VelmaraUI", "DetailLoadHint", "Continúa la cacería desde el último punto guardado en esta ranura.");

		DetailBodyText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "DetailOccupiedFmt", "Ranura {0}\nGuardado: {1}\nAtributos mejorados: {2}\nCombos desbloqueados: {3}\n\n{4}"),
			FText::FromString(Roman),
			FText::FromString(DateText),
			FText::AsNumber(Data.TotalAttributeLevels),
			FText::AsNumber(Data.UnlockedCombosCount),
			ActionHint));
		DetailQuoteText->SetText(NSLOCTEXT("VelmaraUI", "QuoteOccupied", "\"La sangre recuerda cada paso dado bajo el eclipse.\""));
	}
	else
	{
		DetailTitleText->SetText(FText::Format(
			NSLOCTEXT("VelmaraUI", "DetailEmptyTitle", "RANURA {0} — VACÍA"),
			FText::FromString(Roman)));
		DetailBodyText->SetText(CurrentMode == ESaveSlotsMode::NewGame
			? NSLOCTEXT("VelmaraUI", "DetailNewEmpty", "Ranura disponible para iniciar una nueva cacería desde la Catedral del Eclipse.")
			: NSLOCTEXT("VelmaraUI", "DetailLoadEmpty", "No hay ninguna partida guardada en esta ranura."));
		DetailQuoteText->SetText(NSLOCTEXT("VelmaraUI", "QuoteEmpty", "\"Un lienzo de ceniza aguarda el primer juramento.\""));
	}

	if (DeleteSlotButton)
	{
		DeleteSlotButton->SetVisibility(Data.bOccupied ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UVelmaraSaveSlotsWidget::UpdateFooterActions()
{
	if (!FooterBar)
	{
		return;
	}

	const bool bOccupied = CachedSlotsData.IsValidIndex(HighlightedSlotIndex) && CachedSlotsData[HighlightedSlotIndex].bOccupied;
	TArray<FVelmaraFooterAction> Actions;
	Actions.Add(FVelmaraFooterAction(
		TEXT("Confirm"),
		CurrentMode == ESaveSlotsMode::NewGame
			? NSLOCTEXT("VelmaraUI", "FooterStart", "INICIAR")
			: NSLOCTEXT("VelmaraUI", "FooterLoad", "CARGAR"),
		false,
		CurrentMode == ESaveSlotsMode::NewGame || bOccupied));

	if (bOccupied)
	{
		Actions.Add(FVelmaraFooterAction(TEXT("Reset"), NSLOCTEXT("VelmaraUI", "FooterDeleteSlot", "BORRAR RANURA")));
	}
	Actions.Add(FVelmaraFooterAction(TEXT("Back"), NSLOCTEXT("VelmaraUI", "FooterBack", "ATRÁS")));

	FooterBar->SetActions(Actions);
}

void UVelmaraSaveSlotsWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshSlots();
	if (UWidget* Target = NativeGetDesiredFocusTarget())
	{
		Target->SetKeyboardFocus();
	}
}

UWidget* UVelmaraSaveSlotsWidget::NativeGetDesiredFocusTarget() const
{
	if (CurrentMode == ESaveSlotsMode::LoadGame)
	{
		for (int32 Index = 0; Index < SlotCards.Num(); ++Index)
		{
			if (SlotCards[Index] && SlotCards[Index]->IsOccupied())
			{
				return SlotCards[Index];
			}
		}
	}
	return SlotCards.Num() > 0 ? SlotCards[0].Get() : nullptr;
}

bool UVelmaraSaveSlotsWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

FReply UVelmaraSaveSlotsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	if (Key == EKeys::Delete || Key == EKeys::X || Key == EKeys::Gamepad_FaceButton_Left)
	{
		if (CachedSlotsData.IsValidIndex(HighlightedSlotIndex) && CachedSlotsData[HighlightedSlotIndex].bOccupied)
		{
			HandleSlotDeleteRequested(HighlightedSlotIndex);
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UVelmaraSaveSlotsWidget::HandleSlotHighlighted(int32 SlotIndex)
{
	HighlightedSlotIndex = SlotIndex;
	UpdateDetailPanel(SlotIndex);
	UpdateFooterActions();
}

void UVelmaraSaveSlotsWidget::HandleSlotActivated(int32 SlotIndex)
{
	if (!CachedSlotsData.IsValidIndex(SlotIndex))
	{
		return;
	}

	UVelmaraGameInstance* VelmaraGI = Cast<UVelmaraGameInstance>(GetGameInstance());
	if (!VelmaraGI)
	{
		return;
	}

	const FVelmaraSaveSlotViewData& Data = CachedSlotsData[SlotIndex];
	if (CurrentMode == ESaveSlotsMode::NewGame)
	{
		if (!Data.bOccupied)
		{
			VelmaraGI->CreateNewGame(SlotIndex, TEXT("MainLevel"));
		}
		else
		{
			const FString Roman = FVelmaraUITheme::FormatSlotRomanNumeral(SlotIndex);
			TWeakObjectPtr<UVelmaraGameInstance> WeakGI(VelmaraGI);
			UVelmaraConfirmModalWidget::ShowConfirmModal(
				this,
				FText::Format(NSLOCTEXT("VelmaraUI", "OverwriteTitle", "¿SOBRESCRIBIR RANURA {0}?"), FText::FromString(Roman)),
				NSLOCTEXT("VelmaraUI", "OverwriteBody", "Ya existe una partida guardada en esta ranura. Si continúas, el progreso anterior se perderá de forma permanente."),
				NSLOCTEXT("VelmaraUI", "OverwriteConfirm", "SOBRESCRIBIR"),
				NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
				[WeakGI, SlotIndex]()
				{
					if (WeakGI.IsValid())
					{
						WeakGI->CreateNewGame(SlotIndex, TEXT("MainLevel"));
					}
				});
		}
	}
	else if (Data.bOccupied)
	{
		VelmaraGI->LoadGame(SlotIndex);
	}
}

void UVelmaraSaveSlotsWidget::HandleSlotDeleteRequested(int32 SlotIndex)
{
	if (!CachedSlotsData.IsValidIndex(SlotIndex) || !CachedSlotsData[SlotIndex].bOccupied)
	{
		return;
	}

	const FString Roman = FVelmaraUITheme::FormatSlotRomanNumeral(SlotIndex);
	TWeakObjectPtr<UVelmaraSaveSlotsWidget> WeakThis(this);
	UVelmaraConfirmModalWidget::ShowConfirmModal(
		this,
		FText::Format(NSLOCTEXT("VelmaraUI", "DeleteSlotTitle", "¿ELIMINAR RANURA {0}?"), FText::FromString(Roman)),
		NSLOCTEXT("VelmaraUI", "DeleteSlotBody", "Se borrará de forma permanente la partida guardada en esta ranura."),
		NSLOCTEXT("VelmaraUI", "DeleteSlotConfirm", "ELIMINAR"),
		NSLOCTEXT("VelmaraUI", "ModalCancel", "CANCELAR"),
		[WeakThis, SlotIndex]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			if (UGameInstance* GI = WeakThis->GetGameInstance())
			{
				if (USaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<USaveGameSubsystem>())
				{
					SaveSubsystem->DeleteSaveSlot(SlotIndex);
				}
			}
			WeakThis->RefreshSlots();
			if (WeakThis->SlotCards.IsValidIndex(SlotIndex) && WeakThis->SlotCards[SlotIndex])
			{
				WeakThis->SlotCards[SlotIndex]->SetKeyboardFocus();
			}
		});
}

void UVelmaraSaveSlotsWidget::HandleDeleteButtonClicked()
{
	HandleSlotDeleteRequested(HighlightedSlotIndex);
}

void UVelmaraSaveSlotsWidget::HandleBackButtonClicked()
{
	DeactivateWidget();
}
