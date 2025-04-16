#include "Tutorial/InputPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UInputPromptWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
}

void UInputPromptWidget::InitializePrompt(const FInputPromptData& PromptData, bool bUsingGamepad)
{
    if (Text_DisplayName)
    {
        Text_DisplayName->SetText(PromptData.DisplayText);
    }

    if (Image_Icon)
    {
        UTexture2D* Icon = bUsingGamepad ? PromptData.GamepadIcon : PromptData.KeyboardIcon;
        if (Icon)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Icon);
            Brush.ImageSize = FVector2D(64.f, 64.f);
            Image_Icon->SetBrush(Brush);
        }
    }
}