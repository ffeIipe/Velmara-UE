#include "UI/Widgets/HUD/VelmaraTutorialToastWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonRichTextBlock.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/TextLayout.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UObject/UnrealType.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	class SVelmaraInlineKeycap : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SVelmaraInlineKeycap) {}
			SLATE_ARGUMENT(FString, KeyLabel)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			KeycapBrush = FSlateBrush();
			KeycapBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			KeycapBrush.TintColor = FSlateColor(FVelmaraUITheme::Void(0.92f));
			KeycapBrush.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
			KeycapBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			KeycapBrush.OutlineSettings.Width = 1.0f;
			KeycapBrush.OutlineSettings.Color = FSlateColor(FVelmaraUITheme::BoneMuted(0.85f));
			KeycapBrush.OutlineSettings.bUseBrushTransparency = false;

			ChildSlot
			[
				SNew(SBox)
				.Padding(FMargin(3.0f, 0.0f, 3.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.MinDesiredWidth(28.0f)
					.HeightOverride(28.0f)
					[
						SNew(SBorder)
						.BorderImage(&KeycapBrush)
						.Padding(FMargin(7.0f, 0.0f, 7.0f, 0.0f))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(InArgs._KeyLabel))
							.Font(FVelmaraUITheme::GetDisplayFont(15, 60))
							.ColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()))
							.Justification(ETextJustify::Center)
						]
					]
				]
			];
		}

	private:
		FSlateBrush KeycapBrush;
	};

	class FVelmaraKeycapRichTextDecorator : public FRichTextDecorator
	{
	public:
		FVelmaraKeycapRichTextDecorator(URichTextBlock* InOwner)
			: FRichTextDecorator(InOwner)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			return RunParseResult.Name.Equals(TEXT("Key"), ESearchCase::IgnoreCase)
				|| RunParseResult.Name.Equals(TEXT("Keycap"), ESearchCase::IgnoreCase);
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const override
		{
			FString Label = RunInfo.Content.ToString().TrimStartAndEnd();
			if (Label.StartsWith(TEXT("[")) && Label.EndsWith(TEXT("]")) && Label.Len() >= 2)
			{
				Label = Label.Mid(1, Label.Len() - 2).TrimStartAndEnd();
			}
			if (Label.IsEmpty())
			{
				return TSharedPtr<SWidget>();
			}
			return SNew(SVelmaraInlineKeycap).KeyLabel(Label.ToUpper());
		}
	};

	FString ResolveKeyLabelFromImageId(const FString& RawId, bool bGamepad)
	{
		FString CleanId = RawId.TrimStartAndEnd();
		CleanId.ReplaceInline(TEXT("\""), TEXT(""));
		CleanId.ReplaceInline(TEXT("'"), TEXT(""));
		CleanId = CleanId.TrimStartAndEnd();
		if (CleanId.IsEmpty())
		{
			return FString();
		}

		if (CleanId.Equals(TEXT("F_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("RB") : TEXT("F");
		}
		if (CleanId.Equals(TEXT("E_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("A") : TEXT("E");
		}
		if (CleanId.Equals(TEXT("Q_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LB") : TEXT("Q");
		}
		if (CleanId.Equals(TEXT("R_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("Y") : TEXT("R");
		}
		if (CleanId.Equals(TEXT("W_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LS") : TEXT("W");
		}
		if (CleanId.Equals(TEXT("A_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LS") : TEXT("A");
		}
		if (CleanId.Equals(TEXT("S_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LS") : TEXT("S");
		}
		if (CleanId.Equals(TEXT("D_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LS") : TEXT("D");
		}
		if (CleanId.Equals(TEXT("CTRL_Key"), ESearchCase::IgnoreCase) || CleanId.Equals(TEXT("Crtl_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("B") : TEXT("CTRL");
		}
		if (CleanId.Equals(TEXT("SHIFT_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("B") : TEXT("SHIFT");
		}
		if (CleanId.Equals(TEXT("SPACE_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("A") : TEXT("ESPACIO");
		}
		if (CleanId.Equals(TEXT("TAB_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("VIEW") : TEXT("TAB");
		}
		if (CleanId.Equals(TEXT("ESC_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("MENU") : TEXT("ESC");
		}
		if (CleanId.Equals(TEXT("LMB_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("X") : TEXT("CLIC IZQ");
		}
		if (CleanId.Equals(TEXT("RMB_Key"), ESearchCase::IgnoreCase))
		{
			return bGamepad ? TEXT("LT") : TEXT("CLIC DER");
		}

		if (CleanId.EndsWith(TEXT("_Key"), ESearchCase::IgnoreCase))
		{
			CleanId = CleanId.Left(CleanId.Len() - 4);
		}
		CleanId.ReplaceInline(TEXT("_"), TEXT(" "));
		CleanId = CleanId.TrimStartAndEnd();
		return CleanId.ToUpper();
	}
}

TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<UVelmaraTutorialToastWidget>> UVelmaraTutorialToastWidget::ActiveToastWidgets;

UVelmaraKeycapRichTextDecorator::UVelmaraKeycapRichTextDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> UVelmaraKeycapRichTextDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FVelmaraKeycapRichTextDecorator(InOwner));
}

UVelmaraTutorialToastWidget::UVelmaraTutorialToastWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UVelmaraTutorialToastWidget* UVelmaraTutorialToastWidget::GetOrCreate(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (!PC || !LP)
	{
		return nullptr;
	}

	if (const TWeakObjectPtr<UVelmaraTutorialToastWidget>* Found = ActiveToastWidgets.Find(LP))
	{
		if (Found->IsValid())
		{
			return Found->Get();
		}
	}

	UVelmaraTutorialToastWidget* Toast = CreateWidget<UVelmaraTutorialToastWidget>(PC, StaticClass());
	if (Toast)
	{
		ActiveToastWidgets.Add(LP, Toast);
		if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::GetOrCreate(PC))
		{
			Layout->AttachToToastLayer(Toast);
		}
		else
		{
			Toast->AddToPlayerScreen(10);
		}
	}
	return Toast;
}

void UVelmaraTutorialToastWidget::ShowTutorialPages(const UObject* WorldContext, const TArray<FVelmaraTutorialPage>& Pages)
{
	if (Pages.Num() == 0)
	{
		return;
	}

	if (UVelmaraTutorialToastWidget* Toast = GetOrCreate(WorldContext))
	{
		Toast->EnqueuePages(Pages);
	}
}

TSharedRef<SWidget> UVelmaraTutorialToastWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildToastTree();
	}
	return Super::RebuildWidget();
}

void UVelmaraTutorialToastWidget::BuildToastTree()
{
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TutorialToastRoot"));
	RootOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = RootOverlay;

	ToastCardWrapper = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ToastCardWrapper"));
	ToastCardWrapper->SetWidthOverride(460.0f);
	ToastCardWrapper->SetVisibility(ESlateVisibility::Collapsed);
	ToastCardWrapper->SetRenderOpacity(0.0f);
	if (UOverlaySlot* CardSlot = RootOverlay->AddChildToOverlay(ToastCardWrapper))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Right);
		CardSlot->SetVerticalAlignment(VAlign_Center);
		CardSlot->SetPadding(FMargin(0.0f, 0.0f, 64.0f, 0.0f));
	}

	CardPanel = WidgetTree->ConstructWidget<UVelmaraBracketPanel>(UVelmaraBracketPanel::StaticClass(), TEXT("ToastBracketPanel"));
	CardPanel->SetUseMinimalBrackets(true);
	CardPanel->SetBackdropAlpha(0.84f);
	CardPanel->SetShowSignature(false);
	CardPanel->SetContentPadding(FMargin(28.0f, 22.0f, 28.0f, 18.0f));
	ToastCardWrapper->SetContent(CardPanel);

	UVerticalBox* ContentBox = CardPanel->GetContentBox();

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ToastHeaderRow"));
	if (UVerticalBoxSlot* HSlot = ContentBox->AddChildToVerticalBox(HeaderRow))
	{
		HSlot->SetHorizontalAlignment(HAlign_Fill);
		HSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	USizeBox* DiamondSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DiamondSize->SetWidthOverride(12.0f);
	DiamondSize->SetHeightOverride(12.0f);
	UImage* DiamondImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DiamondImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid),
		FVector2D(12.0f, 12.0f),
		FVelmaraUITheme::BloodBright()));
	DiamondSize->SetContent(DiamondImg);
	if (UHorizontalBoxSlot* DSlot = HeaderRow->AddChildToHorizontalBox(DiamondSize))
	{
		DSlot->SetVerticalAlignment(VAlign_Center);
		DSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	HeaderBadgeText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ToastBadgeText"));
	HeaderBadgeText->SetText(NSLOCTEXT("VelmaraUI", "ToastBadge", "CONSEJO"));
	HeaderBadgeText->SetFont(FVelmaraUITheme::GetDisplayFont(13, 220));
	HeaderBadgeText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BloodBright()));
	if (UHorizontalBoxSlot* BSlot = HeaderRow->AddChildToHorizontalBox(HeaderBadgeText))
	{
		BSlot->SetVerticalAlignment(VAlign_Center);
		BSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	PageDotsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ToastPageDots"));
	if (UHorizontalBoxSlot* DotsSlot = HeaderRow->AddChildToHorizontalBox(PageDotsBox))
	{
		DotsSlot->SetVerticalAlignment(VAlign_Center);
		DotsSlot->SetHorizontalAlignment(HAlign_Right);
	}

	TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ToastTitleText"));
	TitleText->SetFont(FVelmaraUITheme::GetDisplayFont(19, 180));
	TitleText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	TitleText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* TSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	USizeBox* SepSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SepSize->SetHeightOverride(14.0f);
	UImage* SepImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	SepImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::OrnSepDiamond),
		FVector2D(380.0f, 14.0f),
		FVelmaraUITheme::BoneMuted(0.75f)));
	SepSize->SetContent(SepImg);
	if (UVerticalBoxSlot* SepSlot = ContentBox->AddChildToVerticalBox(SepSize))
	{
		SepSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	RichBodyText = WidgetTree->ConstructWidget<UCommonRichTextBlock>(UCommonRichTextBlock::StaticClass(), TEXT("ToastRichBody"));
	if (UDataTable* StyleSet = FVelmaraUITheme::LoadRichTextStyleSet())
	{
		RichBodyText->SetTextStyleSet(StyleSet);
	}
	RichBodyText->SetDecorators({ UVelmaraKeycapRichTextDecorator::StaticClass() });
	RichBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(RichBodyText))
	{
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarSize->SetHeightOverride(2.0f);
	TimerBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ToastTimerBar"));
	FProgressBarStyle BarStyle;
	BarStyle.SetBackgroundImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::Ash(0.45f)));
	BarStyle.SetFillImage(FVelmaraUITheme::MakeSolidBrush(FVelmaraUITheme::BloodBright()));
	TimerBar->SetWidgetStyle(BarStyle);
	TimerBar->SetFillColorAndOpacity(FVelmaraUITheme::BloodBright());
	TimerBar->SetPercent(1.0f);
	BarSize->SetContent(TimerBar);
	if (UVerticalBoxSlot* BarSlot = ContentBox->AddChildToVerticalBox(BarSize))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	DismissHintText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("ToastDismissHint"));
	DismissHintText->SetFont(FVelmaraUITheme::GetDisplayFont(12, 160));
	DismissHintText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::BoneMuted(0.85f)));
	DismissHintText->SetJustification(ETextJustify::Right);
	if (UVerticalBoxSlot* HintSlot = ContentBox->AddChildToVerticalBox(DismissHintText))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Right);
	}

	RefreshDismissHint();
}

void UVelmaraTutorialToastWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		ActiveToastWidgets.Add(LP, this);
		if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			InputMethodHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UVelmaraTutorialToastWidget::HandleInputMethodChanged);
		}
	}
	RefreshDismissHint();
}

void UVelmaraTutorialToastWidget::NativeDestruct()
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const TWeakObjectPtr<UVelmaraTutorialToastWidget>* Existing = ActiveToastWidgets.Find(LP))
		{
			if (Existing->Get() == this)
			{
				ActiveToastWidgets.Remove(LP);
			}
		}
		if (InputMethodHandle.IsValid())
		{
			if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
			{
				InputSys->OnInputMethodChangedNative.Remove(InputMethodHandle);
			}
			InputMethodHandle.Reset();
		}
	}

	Super::NativeDestruct();
}

void UVelmaraTutorialToastWidget::EnqueuePages(const TArray<FVelmaraTutorialPage>& Pages)
{
	if (Pages.Num() == 0)
	{
		return;
	}

	if (!bShowingToast)
	{
		ActivePages = Pages;
		CurrentPageIndex = 0;
		bShowingToast = true;
		if (ToastCardWrapper)
		{
			ToastCardWrapper->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		DisplayPage(0);
	}
	else
	{
		ActivePages.Append(Pages);
		DisplayPage(CurrentPageIndex);
	}
}

void UVelmaraTutorialToastWidget::AdvanceOrDismiss()
{
	if (!bShowingToast)
	{
		return;
	}

	const int32 NextIndex = CurrentPageIndex + 1;
	if (ActivePages.IsValidIndex(NextIndex))
	{
		DisplayPage(NextIndex);
	}
	else
	{
		bShowingToast = false;
		CurrentPageIndex = INDEX_NONE;
		ActivePages.Reset();
	}
}

void UVelmaraTutorialToastWidget::DisplayPage(int32 PageIndex)
{
	if (!ActivePages.IsValidIndex(PageIndex))
	{
		bShowingToast = false;
		return;
	}

	CurrentPageIndex = PageIndex;
	const FVelmaraTutorialPage& Page = ActivePages[PageIndex];
	PageTotalSeconds = FMath::Max(2.5f, Page.DurationSeconds);
	PageRemainingSeconds = PageTotalSeconds;

	if (HeaderBadgeText)
	{
		if (ActivePages.Num() > 1)
		{
			HeaderBadgeText->SetText(FText::Format(
				NSLOCTEXT("VelmaraUI", "ToastBadgeMulti", "CONSEJO  ·  {0} / {1}"),
				FText::AsNumber(PageIndex + 1),
				FText::AsNumber(ActivePages.Num())));
		}
		else
		{
			HeaderBadgeText->SetText(NSLOCTEXT("VelmaraUI", "ToastBadge", "CONSEJO"));
		}
	}

	if (TitleText)
	{
		const FText SafeTitle = Page.Title.IsEmpty() ? NSLOCTEXT("VelmaraUI", "ToastDefaultTitle", "CONSEJO DE CACERÍA") : Page.Title;
		TitleText->SetText(SafeTitle.ToUpper());
	}

	if (RichBodyText)
	{
		RichBodyText->SetText(FormatRichBodyForCurrentInput(Page.RichBody));
	}

	if (TimerBar)
	{
		TimerBar->SetPercent(1.0f);
	}

	RefreshPageDots();
	RefreshDismissHint();
}

void UVelmaraTutorialToastWidget::RefreshPageDots()
{
	if (!PageDotsBox || !WidgetTree)
	{
		return;
	}

	PageDotsBox->ClearChildren();
	if (ActivePages.Num() <= 1)
	{
		return;
	}

	UTexture2D* SolidTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondSolid);
	UTexture2D* HollowTex = FVelmaraUITheme::LoadTexture(VelmaraUIPaths::MarkerDiamondHollow);

	for (int32 Index = 0; Index < ActivePages.Num(); ++Index)
	{
		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		DotSize->SetWidthOverride(10.0f);
		DotSize->SetHeightOverride(10.0f);
		UImage* DotImg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		const bool bCurrent = (Index == CurrentPageIndex);
		DotImg->SetBrush(FVelmaraUITheme::MakeImageBrush(
			bCurrent ? SolidTex : HollowTex,
			FVector2D(10.0f, 10.0f),
			bCurrent ? FVelmaraUITheme::BloodBright() : FVelmaraUITheme::Ash()));
		DotSize->SetContent(DotImg);
		if (UHorizontalBoxSlot* DSlot = PageDotsBox->AddChildToHorizontalBox(DotSize))
		{
			DSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

void UVelmaraTutorialToastWidget::RefreshDismissHint()
{
	if (!DismissHintText)
	{
		return;
	}

	bool bGamepad = false;
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			bGamepad = InputSys->GetCurrentInputType() == ECommonInputType::Gamepad;
		}
	}

	const bool bHasNext = ActivePages.IsValidIndex(CurrentPageIndex + 1);
	const FString KeyLabel = bGamepad ? TEXT("[VIEW]") : TEXT("[TAB]");
	const FString ActionLabel = bHasNext ? TEXT("SIGUIENTE") : TEXT("OCULTAR");
	DismissHintText->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), *KeyLabel, *ActionLabel)));
}

FText UVelmaraTutorialToastWidget::FormatRichBodyForCurrentInput(const FText& RawBody) const
{
	bool bGamepad = false;
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (const UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			bGamepad = InputSys->GetCurrentInputType() == ECommonInputType::Gamepad;
		}
	}

	FString Body = RawBody.ToString();

	int32 SearchPos = 0;
	while (SearchPos < Body.Len())
	{
		const int32 ImgStart = Body.Find(TEXT("<img"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchPos);
		if (ImgStart == INDEX_NONE)
		{
			break;
		}

		const int32 TagEnd = Body.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ImgStart);
		if (TagEnd == INDEX_NONE)
		{
			break;
		}

		int32 ReplaceEnd = TagEnd + 1;
		if (Body.Mid(ReplaceEnd, 3).Equals(TEXT("</>"), ESearchCase::CaseSensitive))
		{
			ReplaceEnd += 3;
		}

		const FString TagSlice = Body.Mid(ImgStart, TagEnd - ImgStart + 1);
		const int32 IdAttrIdx = TagSlice.Find(TEXT("id="), ESearchCase::IgnoreCase, ESearchDir::FromStart, 4);
		FString KeyLabel;
		if (IdAttrIdx != INDEX_NONE)
		{
			int32 ValIdx = IdAttrIdx + 3;
			while (ValIdx < TagSlice.Len() && FChar::IsWhitespace(TagSlice[ValIdx]))
			{
				++ValIdx;
			}

			FString RawId;
			if (ValIdx < TagSlice.Len() && (TagSlice[ValIdx] == TCHAR('"') || TagSlice[ValIdx] == TCHAR('\'')))
			{
				const TCHAR QuoteChar = TagSlice[ValIdx];
				++ValIdx;
				const int32 CloseQuoteIdx = TagSlice.Find(FString::Chr(QuoteChar), ESearchCase::CaseSensitive, ESearchDir::FromStart, ValIdx);
				if (CloseQuoteIdx != INDEX_NONE)
				{
					RawId = TagSlice.Mid(ValIdx, CloseQuoteIdx - ValIdx);
				}
			}
			else
			{
				int32 ValEnd = ValIdx;
				while (ValEnd < TagSlice.Len()
					&& !FChar::IsWhitespace(TagSlice[ValEnd])
					&& TagSlice[ValEnd] != TCHAR('/')
					&& TagSlice[ValEnd] != TCHAR('>'))
				{
					++ValEnd;
				}
				RawId = TagSlice.Mid(ValIdx, ValEnd - ValIdx);
			}

			KeyLabel = ResolveKeyLabelFromImageId(RawId, bGamepad);
		}

		if (!KeyLabel.IsEmpty())
		{
			const FString Replacement = FString::Printf(TEXT("<Key>%s</>"), *KeyLabel);
			Body = Body.Left(ImgStart) + Replacement + Body.Mid(ReplaceEnd);
			SearchPos = ImgStart + Replacement.Len();
		}
		else
		{
			SearchPos = ReplaceEnd;
		}
	}

	auto ReplaceToken = [&](const TCHAR* Token, const TCHAR* KbmStr, const TCHAR* PadStr)
	{
		const FString Replacement = FString::Printf(TEXT("<Key>%s</>"), bGamepad ? PadStr : KbmStr);
		Body.ReplaceInline(Token, *Replacement, ESearchCase::IgnoreCase);
	};

	ReplaceToken(TEXT("{Interact}"), TEXT("E"), TEXT("A"));
	ReplaceToken(TEXT("{Possess}"), TEXT("F"), TEXT("RB"));
	ReplaceToken(TEXT("{Spectral}"), TEXT("Q"), TEXT("LB"));
	ReplaceToken(TEXT("{Attack}"), TEXT("CLIC IZQ"), TEXT("X"));
	ReplaceToken(TEXT("{Dodge}"), TEXT("SHIFT"), TEXT("B"));
	ReplaceToken(TEXT("{Jump}"), TEXT("ESPACIO"), TEXT("A"));
	ReplaceToken(TEXT("{Inventory}"), TEXT("I"), TEXT("VIEW"));
	ReplaceToken(TEXT("[E]"), TEXT("E"), TEXT("A"));
	ReplaceToken(TEXT("[F]"), TEXT("F"), TEXT("RB"));
	ReplaceToken(TEXT("[Q]"), TEXT("Q"), TEXT("LB"));
	ReplaceToken(TEXT("[R]"), TEXT("R"), TEXT("Y"));
	ReplaceToken(TEXT("[CTRL]"), TEXT("CTRL"), TEXT("B"));
	ReplaceToken(TEXT("[SHIFT]"), TEXT("SHIFT"), TEXT("B"));
	ReplaceToken(TEXT("[TAB]"), TEXT("TAB"), TEXT("VIEW"));

	Body.ReplaceInline(TEXT("Press E"), *FString::Printf(TEXT("Press <Key>%s</>"), bGamepad ? TEXT("A") : TEXT("E")), ESearchCase::IgnoreCase);
	Body.ReplaceInline(TEXT("Presiona E"), *FString::Printf(TEXT("Presiona <Key>%s</>"), bGamepad ? TEXT("A") : TEXT("E")), ESearchCase::IgnoreCase);

	return FText::FromString(Body);
}

void UVelmaraTutorialToastWidget::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	if (bShowingToast && ActivePages.IsValidIndex(CurrentPageIndex))
	{
		if (RichBodyText)
		{
			RichBodyText->SetText(FormatRichBodyForCurrentInput(ActivePages[CurrentPageIndex].RichBody));
		}
		RefreshDismissHint();
	}
}

void UVelmaraTutorialToastWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bShowingToast)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (PC->WasInputKeyJustPressed(EKeys::Tab) || PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Left))
			{
				AdvanceOrDismiss();
			}
		}

		if (bShowingToast)
		{
			PageRemainingSeconds -= InDeltaTime;
			if (TimerBar && PageTotalSeconds > 0.0f)
			{
				TimerBar->SetPercent(FMath::Clamp(PageRemainingSeconds / PageTotalSeconds, 0.0f, 1.0f));
			}
			if (PageRemainingSeconds <= 0.0f)
			{
				AdvanceOrDismiss();
			}
		}
	}

	const float TargetAlpha = bShowingToast ? 1.0f : 0.0f;
	if (!FMath::IsNearlyEqual(VisibilityAnimAlpha, TargetAlpha, 0.005f))
	{
		const float AnimSpeed = 1.0f / 0.18f;
		VisibilityAnimAlpha = FMath::FInterpConstantTo(VisibilityAnimAlpha, TargetAlpha, InDeltaTime, AnimSpeed);
		if (ToastCardWrapper)
		{
			ToastCardWrapper->SetRenderOpacity(VisibilityAnimAlpha);
			const float SlideOffsetX = (1.0f - VisibilityAnimAlpha) * 16.0f;
			ToastCardWrapper->SetRenderTranslation(FVector2D(SlideOffsetX, 0.0f));
			if (!bShowingToast && VisibilityAnimAlpha <= 0.01f)
			{
				ToastCardWrapper->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

UVelmaraLegacyPromptBridgeWidget::UVelmaraLegacyPromptBridgeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = false;
	bAutoActivate = false;
	bSupportsActivationFocus = false;
}

TOptional<FUIInputConfig> UVelmaraLegacyPromptBridgeWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, true);
}

void UVelmaraLegacyPromptBridgeWidget::ForwardPagesToToast(const TArray<FVelmaraTutorialPage>& Pages)
{
	if (bForwarded || Pages.Num() == 0)
	{
		return;
	}

	bForwarded = true;
	ClearForwardPollTimer();
	UVelmaraTutorialToastWidget::ShowTutorialPages(this, Pages);
	CleanupBridgeWidget();
}

void UVelmaraLegacyPromptBridgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
	ExtractAndForwardFromBlueprintProperties();
	if (!bForwarded)
	{
		ScheduleForwardPoll();
	}
}

void UVelmaraLegacyPromptBridgeWidget::NativeDestruct()
{
	ClearForwardPollTimer();
	Super::NativeDestruct();
}

void UVelmaraLegacyPromptBridgeWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	bForwarded = false;
	TicksWaited = 0;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
	ExtractAndForwardFromBlueprintProperties();
	if (!bForwarded)
	{
		ScheduleForwardPoll();
	}
}

void UVelmaraLegacyPromptBridgeWidget::NativeOnDeactivated()
{
	ClearForwardPollTimer();
	ClearBlueprintTutorialProperties();
	bForwarded = false;
	TicksWaited = 0;
	Super::NativeOnDeactivated();
}

void UVelmaraLegacyPromptBridgeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bForwarded)
	{
		const UWorld* World = GetWorld();
		if (!World || !World->GetTimerManager().IsTimerActive(ForwardPollTimerHandle))
		{
			PollAndForwardOrCleanup();
		}
	}
}

void UVelmaraLegacyPromptBridgeWidget::ScheduleForwardPoll()
{
	if (bForwarded)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(ForwardPollTimerHandle))
		{
			World->GetTimerManager().SetTimer(ForwardPollTimerHandle, this, &UVelmaraLegacyPromptBridgeWidget::PollAndForwardOrCleanup, 0.05f, true, 0.0f);
		}
	}
}

void UVelmaraLegacyPromptBridgeWidget::PollAndForwardOrCleanup()
{
	if (bForwarded)
	{
		ClearForwardPollTimer();
		return;
	}

	ExtractAndForwardFromBlueprintProperties();
	if (!bForwarded)
	{
		++TicksWaited;
		if (TicksWaited >= 10)
		{
			CleanupBridgeWidget();
		}
	}
}

void UVelmaraLegacyPromptBridgeWidget::ClearForwardPollTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ForwardPollTimerHandle);
	}
}

void UVelmaraLegacyPromptBridgeWidget::ClearBlueprintTutorialProperties()
{
	for (TFieldIterator<FArrayProperty> It(GetClass()); It; ++It)
	{
		FArrayProperty* ArrayProp = *It;
		if (!ArrayProp || !ArrayProp->GetName().Contains(TEXT("Tutorial"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
		ArrayHelper.EmptyValues();
	}
}

void UVelmaraLegacyPromptBridgeWidget::CleanupBridgeWidget()
{
	ClearForwardPollTimer();
	ClearBlueprintTutorialProperties();
	SetVisibility(ESlateVisibility::Collapsed);

	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		if (UCommonActivatableWidgetStack* MenuStack = Layout->GetMenuStack())
		{
			MenuStack->RemoveWidget(*this);
		}
		if (IsActivated())
		{
			DeactivateWidget();
		}
		if (IsInViewport() || GetParent() != nullptr)
		{
			RemoveFromParent();
		}
		Layout->RefreshInputMode();
	}
	else
	{
		DeactivateWidget();
		if (IsInViewport() || GetParent() != nullptr)
		{
			RemoveFromParent();
		}
	}

	bForwarded = false;
	TicksWaited = 0;
}

void UVelmaraLegacyPromptBridgeWidget::ExtractAndForwardFromBlueprintProperties()
{
	if (bForwarded)
	{
		return;
	}

	TArray<FVelmaraTutorialPage> ExtractedPages;

	for (TFieldIterator<FArrayProperty> It(GetClass()); It; ++It)
	{
		FArrayProperty* ArrayProp = *It;
		if (!ArrayProp || !ArrayProp->GetName().Contains(TEXT("Tutorial"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
		if (!InnerStructProp || !InnerStructProp->Struct)
		{
			continue;
		}

		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
		for (int32 ElemIdx = 0; ElemIdx < ArrayHelper.Num(); ++ElemIdx)
		{
			const void* ElemPtr = ArrayHelper.GetRawPtr(ElemIdx);
			FVelmaraTutorialPage Page;
			TArray<FText> FallbackTexts;

			for (TFieldIterator<FProperty> FieldIt(InnerStructProp->Struct); FieldIt; ++FieldIt)
			{
				FProperty* Field = *FieldIt;
				FText ExtractedText;
				if (const FTextProperty* TextProp = CastField<FTextProperty>(Field))
				{
					ExtractedText = TextProp->GetPropertyValue_InContainer(ElemPtr);
				}
				else if (const FStrProperty* StrProp = CastField<FStrProperty>(Field))
				{
					ExtractedText = FText::FromString(StrProp->GetPropertyValue_InContainer(ElemPtr));
				}

				if (ExtractedText.IsEmpty())
				{
					continue;
				}

				const FString AuthoredName = Field->GetAuthoredName();
				if (AuthoredName.Contains(TEXT("Title"), ESearchCase::IgnoreCase) || AuthoredName.Contains(TEXT("Header"), ESearchCase::IgnoreCase))
				{
					Page.Title = ExtractedText;
				}
				else if (AuthoredName.Contains(TEXT("Desc"), ESearchCase::IgnoreCase)
					|| AuthoredName.Contains(TEXT("Parragraph"), ESearchCase::IgnoreCase)
					|| AuthoredName.Contains(TEXT("Paragraph"), ESearchCase::IgnoreCase)
					|| AuthoredName.Contains(TEXT("Body"), ESearchCase::IgnoreCase)
					|| AuthoredName.Contains(TEXT("Text"), ESearchCase::IgnoreCase)
					|| AuthoredName.Contains(TEXT("Prompt"), ESearchCase::IgnoreCase))
				{
					Page.RichBody = ExtractedText;
				}
				else
				{
					FallbackTexts.Add(ExtractedText);
				}
			}

			if (Page.Title.IsEmpty() && FallbackTexts.Num() >= 2)
			{
				Page.Title = FallbackTexts[0];
				if (Page.RichBody.IsEmpty())
				{
					Page.RichBody = FallbackTexts[1];
				}
			}
			else if (Page.RichBody.IsEmpty() && FallbackTexts.Num() >= 1)
			{
				Page.RichBody = FallbackTexts[0];
			}

			if (!Page.Title.IsEmpty() || !Page.RichBody.IsEmpty())
			{
				ExtractedPages.Add(Page);
			}
		}
	}

	if (ExtractedPages.Num() > 0)
	{
		ForwardPagesToToast(ExtractedPages);
	}
}
