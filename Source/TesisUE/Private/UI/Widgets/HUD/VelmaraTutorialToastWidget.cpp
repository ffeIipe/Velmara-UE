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
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"
#include "UI/Widgets/Common/VelmaraBracketPanel.h"
#include "UObject/UnrealType.h"

TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<UVelmaraTutorialToastWidget>> UVelmaraTutorialToastWidget::ActiveToastWidgets;

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
	RootOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
			ToastCardWrapper->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
	auto ReplaceToken = [&](const TCHAR* Token, const TCHAR* KbmStr, const TCHAR* PadStr)
	{
		const FString Replacement = FString::Printf(TEXT("<Key>[%s]</>"), bGamepad ? PadStr : KbmStr);
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
	ReplaceToken(TEXT("Press E"), TEXT("E"), TEXT("A"));
	ReplaceToken(TEXT("Presiona E"), TEXT("E"), TEXT("A"));

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
}

TOptional<FUIInputConfig> UVelmaraLegacyPromptBridgeWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, false);
}

void UVelmaraLegacyPromptBridgeWidget::ForwardPagesToToast(const TArray<FVelmaraTutorialPage>& Pages)
{
	if (Pages.Num() == 0)
	{
		return;
	}
	bForwarded = true;
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UVelmaraTutorialToastWidget::ShowTutorialPages(this, Pages);
	DeactivateWidget();
	RemoveFromParent();
	if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(this))
	{
		Layout->RefreshInputMode();
	}
}

void UVelmaraLegacyPromptBridgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	ExtractAndForwardFromBlueprintProperties();
}

void UVelmaraLegacyPromptBridgeWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::Collapsed);
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	ExtractAndForwardFromBlueprintProperties();
}

void UVelmaraLegacyPromptBridgeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bForwarded)
	{
		ExtractAndForwardFromBlueprintProperties();
		++TicksWaited;
		if (TicksWaited >= 4)
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			DeactivateWidget();
			RemoveFromParent();
		}
	}
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
