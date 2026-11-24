#include "Tutorial/PromptWidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/InputPromptWidget.h"

UPromptWidgetComponent::UPromptWidgetComponent()
{
	bUseGamepadIcon = false;

	static ConstructorHelpers::FObjectFinder<UDataTable> InputDataTable(TEXT("/Game/Blueprints/DataTables/DT_InputPrompts.DT_InputPrompts"));
	if (InputDataTable.Succeeded())
	{
		PromptDataTable = InputDataTable.Object;
	}

	SetDrawAtDesiredSize(true);
	SetWidgetSpace(EWidgetSpace::Screen);
}

void UPromptWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
			{
				bUseGamepadIcon = (InputSys->GetCurrentInputType() == ECommonInputType::Gamepad);
				InputMethodHandle = InputSys->OnInputMethodChangedNative.AddUObject(this, &UPromptWidgetComponent::HandleInputMethodChanged);
			}
		}
	}

	if (AActor* Owner = GetOwner())
	{
		if (UTotemChargeComponent* TotemCharge = Owner->FindComponentByClass<UTotemChargeComponent>())
		{
			TotemCharge->OnChargeChanged.AddUniqueDynamic(this, &UPromptWidgetComponent::HandleTotemChargeChanged);
		}
	}

	LoadAndApplyPrompt();
}

void UPromptWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InputMethodHandle.IsValid())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				if (UCommonInputSubsystem* InputSys = UCommonInputSubsystem::Get(LP))
				{
					InputSys->OnInputMethodChangedNative.Remove(InputMethodHandle);
				}
			}
		}
		InputMethodHandle.Reset();
	}

	if (AActor* Owner = GetOwner())
	{
		if (UTotemChargeComponent* TotemCharge = Owner->FindComponentByClass<UTotemChargeComponent>())
		{
			TotemCharge->OnChargeChanged.RemoveDynamic(this, &UPromptWidgetComponent::HandleTotemChargeChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UPromptWidgetComponent::LoadAndApplyPrompt()
{
	if (!PromptDataTable || PromptRowName.IsNone()) return;

	const FInputPromptData* PromptRow = PromptDataTable->FindRow<FInputPromptData>(PromptRowName, TEXT("Prompt Load"));
	if (!PromptRow) return;

	if (!GetWidget())
	{
		TSubclassOf<UUserWidget> ClassToCreate = GetWidgetClass();
		if (!ClassToCreate)
		{
			ClassToCreate = UInputPromptWidget::StaticClass();
		}
		if (UUserWidget* NewWidget = CreateWidget<UUserWidget>(GetWorld(), ClassToCreate))
		{
			SetWidget(NewWidget);
		}
	}

	if (UInputPromptWidget* PromptWidget = Cast<UInputPromptWidget>(GetWidget()))
	{
		PromptWidget->InitializePrompt(*PromptRow, bUseGamepadIcon);
		if (AActor* Owner = GetOwner())
		{
			if (const UTotemChargeComponent* TotemCharge = Owner->FindComponentByClass<UTotemChargeComponent>())
			{
				PromptWidget->SetHoldProgress(TotemCharge->GetChargePercent());
			}
		}
	}
}

void UPromptWidgetComponent::HandleInputMethodChanged(ECommonInputType NewInputType)
{
	bUseGamepadIcon = (NewInputType == ECommonInputType::Gamepad);
	LoadAndApplyPrompt();
}

void UPromptWidgetComponent::HandleTotemChargeChanged(int32 CurrentKills, int32 KillsToCharge)
{
	if (UInputPromptWidget* PromptWidget = Cast<UInputPromptWidget>(GetWidget()))
	{
		const float Percent = KillsToCharge > 0 ? FMath::Clamp(static_cast<float>(CurrentKills) / static_cast<float>(KillsToCharge), 0.0f, 1.0f) : 1.0f;
		PromptWidget->SetHoldProgress(Percent);
	}
}

void UPromptWidgetComponent::EnablePromptWidget()
{
	if (GetWidget())
	{
		GetWidget()->SetVisibility(ESlateVisibility::Visible);
	}
	SetHiddenInGame(false);
	SetVisibility(true);
}

void UPromptWidgetComponent::DisablePromptWidget()
{
	if (GetWidget())
	{
		GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}
	SetHiddenInGame(true);
	SetVisibility(false);
}
