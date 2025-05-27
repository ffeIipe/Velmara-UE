#include "Tutorial/TutorialWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "MediaPlayer.h"
#include "Kismet/GameplayStatics.h"

void UTutorialWidget::InitializeTutorial(UMediaSource* VideoSource, FText TitleText, FText ParragraphText, AActor* OwningTrigger)
{
    if (TutorialTitleTextBlock)
        TutorialTitleTextBlock->SetText(TitleText);
    
    if (TutorialParragraphTextBlock)
        TutorialParragraphTextBlock->SetText(ParragraphText);

    if (MediaPlayer && VideoSource)
    {
        MediaPlayer->SetLooping(true);
        if (MediaPlayer->OpenSource(VideoSource))
        {
            MediaPlayer->Play();
        }
    }

    TriggerOwner = OwningTrigger;
}

void UTutorialWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UTutorialWidget::OnCloseButtonClicked);
}

void UTutorialWidget::OnCloseButtonClicked()
{
    if (MediaPlayer && MediaPlayer->IsPlaying())
        MediaPlayer->Pause();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetPause(false);
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }

    RemoveFromParent();
}

void UTutorialWidget::PlayFadeIn()
{
    if (FadeInOut)
    {
        PlayAnimation(FadeInOut, 0.f, 1, EUMGSequencePlayMode::Forward);
    }
}
void UTutorialWidget::PlayFadeOut()
{
    if (FadeInOut)
    {
        PlayAnimation(FadeInOut, 0.f, 1, EUMGSequencePlayMode::Reverse);
    }
}
