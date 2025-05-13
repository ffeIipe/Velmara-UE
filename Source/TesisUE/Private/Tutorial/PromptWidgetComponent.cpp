#include "Tutorial/PromptWidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/InputPromptWidget.h"

UPromptWidgetComponent::UPromptWidgetComponent()
{
    bUseGamepadIcon = false;
    PromptDataTable = nullptr;
    PromptRowName = NAME_None;
}

void UPromptWidgetComponent::BeginPlay()
{
    Super::BeginPlay();
    LoadAndApplyPrompt();
}

void UPromptWidgetComponent::LoadAndApplyPrompt()
{
    if (!PromptDataTable || PromptRowName.IsNone()) return;

    const FInputPromptData* PromptRow = PromptDataTable->FindRow<FInputPromptData>(PromptRowName, TEXT("Prompt Load"));

    if (!PromptRow) return;

    if (!GetWidget())
    {
        UUserWidget* NewWidget = CreateWidget<UUserWidget>(GetWorld(), GetWidgetClass());
        if (NewWidget)
        {
            SetWidget(NewWidget);
        }
    }

    if (UInputPromptWidget* PromptWidget = Cast<UInputPromptWidget>(GetWidget()))
    {
        PromptWidget->InitializePrompt(*PromptRow, bUseGamepadIcon);
    }
}

void UPromptWidgetComponent::EnablePromptWidget(bool bIsEnable)
{
    bIsEnable ? 
        GetWidget()->SetVisibility(ESlateVisibility::Visible) : 
        GetWidget()->SetVisibility(ESlateVisibility::Hidden);

    bIsEnable ?
        SetVisibility(true) :
        SetVisibility(false);

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString(" UPromptWidgetComponent::EnablePromptWidget"));
}