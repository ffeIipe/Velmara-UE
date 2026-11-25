#include "UI/Widgets/HUD/VelmaraWorldPromptsWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CapsuleComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Entities/Entity.h"
#include "Features/InventorySystem/Interfaces/Pickable.h"
#include "Features/InventorySystem/Items/Item.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "Features/TotemSystem/Subsystems/TotemSubsystem.h"
#include "GAS/Abilities/BloodSuckAbility.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InteractionSystem/Interfaces/Interactable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/CustomPlayerController.h"
#include "SpectralMode/Interfaces/Spectral.h"
#include "Subsystems/BloodMarkSubsystem.h"
#include "Subsystems/SpectralObjectsSubsystem.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Style/VelmaraUITheme.h"

namespace
{
	FLinearColor GetPromptAccentColor(EVelmaraWorldPromptKind Kind)
	{
		switch (Kind)
		{
		case EVelmaraWorldPromptKind::BloodSuck:
			return FVelmaraUITheme::BloodBright();
		case EVelmaraWorldPromptKind::Possess:
			return FVelmaraUITheme::Spectral();
		case EVelmaraWorldPromptKind::Interact:
		default:
			return FVelmaraUITheme::BoneMuted(0.95f);
		}
	}

	FSlateBrush MakePromptBoxBrush(const FLinearColor& FillColor, const FLinearColor& OutlineColor)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.Width = 1.0f;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}

TSharedRef<SWidget> UVelmaraWorldPromptStackWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PromptStackRoot"));
		RootVBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = RootVBox;
		if (bEntriesInitialized && CachedEntries.Num() > 0)
		{
			RebuildRows();
		}
	}
	return Super::RebuildWidget();
}

void UVelmaraWorldPromptStackWidget::SetEntries(const TArray<FVelmaraWorldPromptEntry>& NewEntries)
{
	if (bEntriesInitialized && CachedEntries == NewEntries)
	{
		return;
	}

	CachedEntries = NewEntries;
	bEntriesInitialized = true;

	if (RootVBox)
	{
		RebuildRows();
	}
}

void UVelmaraWorldPromptStackWidget::ResetFade()
{
	FadeAlpha = 0.0f;
	SetRenderOpacity(0.0f);
}

void UVelmaraWorldPromptStackWidget::UpdateFade(float DeltaTime)
{
	if (FadeAlpha < 0.999f)
	{
		FadeAlpha = FMath::FInterpTo(FadeAlpha, 1.0f, DeltaTime, 14.0f);
		if (FadeAlpha > 0.995f)
		{
			FadeAlpha = 1.0f;
		}
		SetRenderOpacity(FadeAlpha);
	}
}

void UVelmaraWorldPromptStackWidget::RebuildRows()
{
	if (!RootVBox || !WidgetTree)
	{
		return;
	}

	RootVBox->ClearChildren();

	for (int32 Index = 0; Index < CachedEntries.Num(); ++Index)
	{
		UBorder* RowWidget = BuildPromptRow(CachedEntries[Index]);
		if (UVerticalBoxSlot* RowSlot = RootVBox->AddChildToVerticalBox(RowWidget))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			const float BottomPad = (Index + 1 < CachedEntries.Num()) ? 5.0f : 0.0f;
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPad));
		}
	}
}

UBorder* UVelmaraWorldPromptStackWidget::BuildPromptRow(const FVelmaraWorldPromptEntry& Entry)
{
	const FLinearColor Accent = GetPromptAccentColor(Entry.Kind);

	UBorder* OuterCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	OuterCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	OuterCard->SetBrush(MakePromptBoxBrush(FVelmaraUITheme::Void(0.10f), FVelmaraUITheme::WithAlpha(Accent, 0.55f)));
	OuterCard->SetPadding(FMargin(0.0f));

	UOverlay* CardOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	CardOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	OuterCard->SetContent(CardOverlay);

	UImage* GradBg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	GradBg->SetVisibility(ESlateVisibility::HitTestInvisible);
	GradBg->SetBrush(FVelmaraUITheme::MakeImageBrush(
		FVelmaraUITheme::LoadTexture(VelmaraUIPaths::GradVFadeBottomUp),
		FVector2D(32.0f, 32.0f),
		FVelmaraUITheme::Void(0.444f)));
	GradBg->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	GradBg->SetRenderScale(FVector2D(1.0f, -1.0f));
	if (UOverlaySlot* GradSlot = CardOverlay->AddChildToOverlay(GradBg))
	{
		GradSlot->SetHorizontalAlignment(HAlign_Fill);
		GradSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* ContentSlot = CardOverlay->AddChildToOverlay(RowBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		ContentSlot->SetPadding(FMargin(6.0f, 4.0f, 10.0f, 4.0f));
	}

	USizeBox* AccentBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	AccentBarSize->SetWidthOverride(2.5f);
	AccentBarSize->SetHeightOverride(22.0f);
	UImage* AccentBar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	AccentBar->SetBrush(FVelmaraUITheme::MakeSolidBrush(Accent));
	AccentBarSize->SetContent(AccentBar);
	if (UHorizontalBoxSlot* BarSlot = RowBox->AddChildToHorizontalBox(AccentBarSize))
	{
		BarSlot->SetVerticalAlignment(VAlign_Center);
		BarSlot->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
	}

	USizeBox* KeycapSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	KeycapSize->SetMinDesiredWidth(24.0f);
	KeycapSize->SetHeightOverride(24.0f);
	UBorder* KeycapBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	KeycapBorder->SetBrush(MakePromptBoxBrush(FVelmaraUITheme::Void(0.95f), FVelmaraUITheme::WithAlpha(Accent, 0.90f)));
	KeycapBorder->SetPadding(FMargin(6.0f, 0.0f));
	KeycapBorder->SetHorizontalAlignment(HAlign_Center);
	KeycapBorder->SetVerticalAlignment(VAlign_Center);
	KeycapSize->SetContent(KeycapBorder);

	UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	KeyText->SetText(FText::FromString(Entry.KeyLabel));
	KeyText->SetFont(FVelmaraUITheme::GetDisplayFont(13, 60));
	KeyText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	KeyText->SetJustification(ETextJustify::Center);
	KeycapBorder->SetContent(KeyText);

	if (UHorizontalBoxSlot* KeySlot = RowBox->AddChildToHorizontalBox(KeycapSize))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ActionText->SetText(Entry.ActionLabel.ToUpper());
	ActionText->SetFont(FVelmaraUITheme::GetDisplayFont(13, 180));
	ActionText->SetColorAndOpacity(FSlateColor(FVelmaraUITheme::Bone()));
	ActionText->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(ActionText))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	return OuterCard;
}

UVelmaraWorldPromptsWidget::UVelmaraWorldPromptsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UVelmaraWorldPromptsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WorldPromptsCanvas"));
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = RootCanvas;
	}
	return Super::RebuildWidget();
}

void UVelmaraWorldPromptsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
		{
			bUsingGamepad = (InputSys->GetCurrentInputType() == ECommonInputType::Gamepad);
			InputMethodHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UVelmaraWorldPromptsWidget::HandleInputMethodChanged);
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UBloodMarkSubsystem* BloodMarks = World->GetSubsystem<UBloodMarkSubsystem>())
		{
			BloodMarks->OnActorBloodMarked.AddUniqueDynamic(this, &UVelmaraWorldPromptsWidget::HandleBloodMarkChanged);
			BloodMarks->OnActorBloodMarkCleared.AddUniqueDynamic(this, &UVelmaraWorldPromptsWidget::HandleBloodMarkChanged);
		}
	}

	bDirtyScan = true;
}

void UVelmaraWorldPromptsWidget::NativeDestruct()
{
	if (InputMethodHandle.IsValid())
	{
		if (ULocalPlayer* LP = GetOwningLocalPlayer())
		{
			if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
			{
				InputSys->OnInputMethodChangedNative.Remove(InputMethodHandle);
			}
		}
		InputMethodHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		if (UBloodMarkSubsystem* BloodMarks = World->GetSubsystem<UBloodMarkSubsystem>())
		{
			BloodMarks->OnActorBloodMarked.RemoveDynamic(this, &UVelmaraWorldPromptsWidget::HandleBloodMarkChanged);
			BloodMarks->OnActorBloodMarkCleared.RemoveDynamic(this, &UVelmaraWorldPromptsWidget::HandleBloodMarkChanged);
		}
	}

	ReleaseAllStacks();
	Super::NativeDestruct();
}

void UVelmaraWorldPromptsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->GetPawn())
	{
		ReleaseAllStacks();
		return;
	}

	if (const UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::Get(GetWorld()); Layout && Layout->HasActiveMenuOrModal())
	{
		ReleaseAllStacks();
		return;
	}

	ScanTimer += InDeltaTime;
	if (bDirtyScan || ScanTimer >= ScanInterval)
	{
		ScanTimer = 0.0f;
		bDirtyScan = false;
		RefreshPromptTargets(PC);
	}

	UpdateScreenPositions(PC, InDeltaTime);
}

void UVelmaraWorldPromptsWidget::RefreshPromptTargets(APlayerController* PC)
{
	UWorld* World = GetWorld();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		ReleaseAllStacks();
		return;
	}

	const ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PC);
	const bool bPossessing = CustomPC && CustomPC->IsPossessing();
	const FVector PawnLoc = Pawn->GetActorLocation();

	const FString InteractKey = bUsingGamepad ? TEXT("A") : TEXT("E");
	const FString BloodSuckKey = bUsingGamepad ? TEXT("RB") : TEXT("F");
	const FString PossessKey = bUsingGamepad ? TEXT("LB") : TEXT("Q");

	TMap<TWeakObjectPtr<AActor>, TArray<FVelmaraWorldPromptEntry>> TargetPrompts;

	TArray<AActor*> OverlapActors;
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_WorldStatic),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic),
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_PhysicsBody)
	};
	UKismetSystemLibrary::SphereOverlapActors(World, PawnLoc, InteractRange, ObjectTypes, AActor::StaticClass(), { Pawn }, OverlapActors);

	const USpectralObjectsSubsystem* SpectralObjects = World->GetSubsystem<USpectralObjectsSubsystem>();
	AActor* BestInteractable = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : OverlapActors)
	{
		if (!IsValid(Candidate) || Candidate->IsHidden())
		{
			continue;
		}

		const bool bIsInteractable = Candidate->Implements<UInteractable>();
		bool bIsUncollectedPickup = false;
		if (!bIsInteractable && Candidate->Implements<UPickable>())
		{
			const AItem* Item = Cast<AItem>(Candidate);
			const bool bNotUsed = !Item || !Item->bWasUsed;
			const bool bOwnedByPawn = Candidate->GetOwner() && Candidate->GetOwner()->IsA<APawn>();
			bIsUncollectedPickup = (Candidate->GetAttachParentActor() == nullptr) && bNotUsed && !bOwnedByPawn;
		}

		if (!bIsInteractable && !bIsUncollectedPickup)
		{
			continue;
		}
		if (Candidate->Implements<USpectral>() && (!SpectralObjects || !SpectralObjects->IsActorRevealed(Candidate)))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), PawnLoc);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestInteractable = Candidate;
		}
	}

	if (BestInteractable)
	{
		FText Label = NSLOCTEXT("VelmaraUI", "PromptInteract", "INTERACTUAR");
		if (const IInteractable* NativeInteractable = Cast<IInteractable>(BestInteractable))
		{
			const FText CustomText = NativeInteractable->GetInteractionText();
			if (!CustomText.IsEmpty() && !CustomText.ToString().Equals(TEXT("Interact"), ESearchCase::IgnoreCase))
			{
				Label = CustomText;
			}
		}

		FVelmaraWorldPromptEntry& Entry = TargetPrompts.FindOrAdd(BestInteractable).AddDefaulted_GetRef();
		Entry.Kind = EVelmaraWorldPromptKind::Interact;
		Entry.KeyLabel = InteractKey;
		Entry.ActionLabel = Label;
	}

	if (!bPossessing)
	{
		if (AEntity* SelfEntity = Cast<AEntity>(Pawn))
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn); ASC && ASC->AbilityActorInfo.IsValid())
			{
				for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
				{
					const UBloodSuckAbility* BloodSuckAbility = Cast<UBloodSuckAbility>(Spec.Ability);
					if (!BloodSuckAbility)
					{
						continue;
					}
					if (BloodSuckAbility->CanActivateAbility(Spec.Handle, ASC->AbilityActorInfo.Get()))
					{
						if (AEntity* Victim = BloodSuckAbility->FindVictim(SelfEntity))
						{
							FVelmaraWorldPromptEntry& Entry = TargetPrompts.FindOrAdd(Victim).AddDefaulted_GetRef();
							Entry.Kind = EVelmaraWorldPromptKind::BloodSuck;
							Entry.KeyLabel = BloodSuckKey;
							Entry.ActionLabel = NSLOCTEXT("VelmaraUI", "PromptBloodSuck", "CHUPAR SANGRE");
						}
					}
					break;
				}
			}
		}

		if (CustomPC)
		{
			const float MaxPossessDistSq = FMath::Square(FMath::Max(CustomPC->PossessionHoverTraceLength, 100.0f));

			auto TryAddPossessTarget = [&](AActor* Candidate)
			{
				if (!IsValid(Candidate) || FVector::DistSquared(Candidate->GetActorLocation(), PawnLoc) > MaxPossessDistSq)
				{
					return;
				}
				if (!CustomPC->CanPossessActor(Candidate))
				{
					return;
				}
				TArray<FVelmaraWorldPromptEntry>& Entries = TargetPrompts.FindOrAdd(Candidate);
				for (const FVelmaraWorldPromptEntry& Existing : Entries)
				{
					if (Existing.Kind == EVelmaraWorldPromptKind::Possess)
					{
						return;
					}
				}
				FVelmaraWorldPromptEntry& Entry = Entries.AddDefaulted_GetRef();
				Entry.Kind = EVelmaraWorldPromptKind::Possess;
				Entry.KeyLabel = PossessKey;
				Entry.ActionLabel = NSLOCTEXT("VelmaraUI", "PromptPossess", "POSEER");
			};

			if (const UBloodMarkSubsystem* BloodMarks = World->GetSubsystem<UBloodMarkSubsystem>())
			{
				TArray<AActor*> MarkedActors;
				BloodMarks->GetMarkedActors(MarkedActors);
				for (AActor* Candidate : MarkedActors)
				{
					TryAddPossessTarget(Candidate);
				}
			}

			if (const UTotemSubsystem* TotemsSys = World->GetSubsystem<UTotemSubsystem>())
			{
				TArray<UTotemChargeComponent*> Totems;
				TotemsSys->GetRegisteredTotems(Totems);
				for (UTotemChargeComponent* TotemComp : Totems)
				{
					if (TotemComp)
					{
						TryAddPossessTarget(TotemComp->GetOwner());
					}
				}
			}
		}
	}

	TArray<TWeakObjectPtr<AActor>> StaleActors;
	for (const TPair<TWeakObjectPtr<AActor>, TObjectPtr<UVelmaraWorldPromptStackWidget>>& Pair : ActiveStacks)
	{
		if (!Pair.Key.IsValid() || !TargetPrompts.Contains(Pair.Key))
		{
			StaleActors.Add(Pair.Key);
		}
	}

	for (const TWeakObjectPtr<AActor>& StaleActor : StaleActors)
	{
		if (TObjectPtr<UVelmaraWorldPromptStackWidget>* FoundStack = ActiveStacks.Find(StaleActor))
		{
			ReleaseStackWidget(FoundStack->Get());
		}
		ActiveStacks.Remove(StaleActor);
		ActiveSlots.Remove(StaleActor);
	}

	for (const TPair<TWeakObjectPtr<AActor>, TArray<FVelmaraWorldPromptEntry>>& Pair : TargetPrompts)
	{
		UVelmaraWorldPromptStackWidget* StackWidget = nullptr;
		if (TObjectPtr<UVelmaraWorldPromptStackWidget>* Existing = ActiveStacks.Find(Pair.Key))
		{
			StackWidget = Existing->Get();
		}
		else
		{
			StackWidget = AcquireStackWidget();
			if (StackWidget)
			{
				StackWidget->ResetFade();
				ActiveStacks.Add(Pair.Key, StackWidget);
				if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(StackWidget->Slot))
				{
					PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
					PanelSlot->SetAutoSize(true);
					ActiveSlots.Add(Pair.Key, PanelSlot);
				}
			}
		}

		if (StackWidget)
		{
			StackWidget->SetEntries(Pair.Value);
		}
	}
}

void UVelmaraWorldPromptsWidget::UpdateScreenPositions(APlayerController* PC, float DeltaTime)
{
	for (const TPair<TWeakObjectPtr<AActor>, TObjectPtr<UVelmaraWorldPromptStackWidget>>& Pair : ActiveStacks)
	{
		UVelmaraWorldPromptStackWidget* StackWidget = Pair.Value.Get();
		const AActor* TargetActor = Pair.Key.Get();
		if (!StackWidget)
		{
			continue;
		}
		if (!IsValid(TargetActor))
		{
			StackWidget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		FVector2D ScreenPos = FVector2D::ZeroVector;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, GetActorPromptAnchor(TargetActor), ScreenPos, false))
		{
			StackWidget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		StackWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (TObjectPtr<UCanvasPanelSlot>* SlotPtr = ActiveSlots.Find(Pair.Key); SlotPtr && SlotPtr->Get())
		{
			SlotPtr->Get()->SetPosition(ScreenPos);
		}
		StackWidget->UpdateFade(DeltaTime);
	}
}

FVector UVelmaraWorldPromptsWidget::GetActorPromptAnchor(const AActor* Actor) const
{
	if (const ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			return Actor->GetActorLocation() + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() + 34.0f);
		}
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	Actor->GetActorBounds(true, Origin, Extent);
	return FVector(Origin.X, Origin.Y, Origin.Z + Extent.Z + 24.0f);
}

UVelmaraWorldPromptStackWidget* UVelmaraWorldPromptsWidget::AcquireStackWidget()
{
	if (!RootCanvas)
	{
		return nullptr;
	}

	if (PooledStacks.Num() > 0)
	{
		return PooledStacks.Pop();
	}

	UVelmaraWorldPromptStackWidget* Created = CreateWidget<UVelmaraWorldPromptStackWidget>(GetOwningPlayer(), UVelmaraWorldPromptStackWidget::StaticClass());
	if (Created)
	{
		Created->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* NewSlot = RootCanvas->AddChildToCanvas(Created))
		{
			NewSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			NewSlot->SetAutoSize(true);
		}
	}
	return Created;
}

void UVelmaraWorldPromptsWidget::ReleaseStackWidget(UVelmaraWorldPromptStackWidget* StackWidget)
{
	if (StackWidget)
	{
		StackWidget->SetVisibility(ESlateVisibility::Collapsed);
		PooledStacks.AddUnique(StackWidget);
	}
}

void UVelmaraWorldPromptsWidget::ReleaseAllStacks()
{
	for (const TPair<TWeakObjectPtr<AActor>, TObjectPtr<UVelmaraWorldPromptStackWidget>>& Pair : ActiveStacks)
	{
		ReleaseStackWidget(Pair.Value.Get());
	}
	ActiveStacks.Empty();
	ActiveSlots.Empty();
}

void UVelmaraWorldPromptsWidget::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	bUsingGamepad = (NewInputType == ECommonInputType::Gamepad);
	bDirtyScan = true;
}

void UVelmaraWorldPromptsWidget::HandleBloodMarkChanged(AActor* Actor)
{
	bDirtyScan = true;
}
