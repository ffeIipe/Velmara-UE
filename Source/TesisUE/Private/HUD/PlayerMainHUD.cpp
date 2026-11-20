#include "HUD/PlayerMainHUD.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "HUD/PlayerMainWidget.h"

void APlayerMainHUD::BeginPlay()
{
    Super::BeginPlay();

}

void APlayerMainHUD::TogglePlayerUI(bool Bool)
{
    if (Bool)
    {
        if (!PlayerMainWidgetInstance)
        {
            PlayerMainWidgetInstance = CreateWidget<UPlayerMainWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerMainWidgetClass);
            PlayerMainWidgetInstance->AddToViewport();
        }
        else
        {
            PlayerMainWidgetInstance->AddToViewport();
        }
    }
    else
    {
        if (PlayerMainWidgetInstance)
        {
            PlayerMainWidgetInstance->RemoveFromParent();
            PlayerMainWidgetInstance = nullptr;
        }
    }
}
