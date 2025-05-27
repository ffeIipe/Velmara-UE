// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidget.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API UTutorialWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable)
    void InitializeTutorial(UMediaSource* VideoSource, FText TitleText , FText ParragraphText, AActor* OwningTrigger);

    void PlayFadeIn();

    void PlayFadeOut();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TutorialTitleTextBlock;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TutorialParragraphTextBlock;

    UPROPERTY(meta = (BindWidget))
    class UButton* CloseButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
    class UMediaPlayer* MediaPlayer;

    UPROPERTY(meta = (BindWidget))
    class UImage* VideoImage;

    UPROPERTY(EditAnywhere, Category = "Tutorial")
    class UMediaTexture* MediaTexture;

    UFUNCTION()
    void OnCloseButtonClicked();

    TWeakObjectPtr<AActor> TriggerOwner;

    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* FadeInOut;
};