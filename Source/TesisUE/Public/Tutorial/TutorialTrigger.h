#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialTrigger.generated.h"

UCLASS()
class TESISUE_API ATutorialTrigger : public AActor
{
	GENERATED_BODY()
	
public:
    ATutorialTrigger();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnPlayerEnter(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(EditAnywhere, Category = "Tutorial")
    class UBoxComponent* TriggerVolume;

    UPROPERTY(EditAnywhere, Category = "Tutorial")
    class UMediaSource* TutorialVideo;

    UPROPERTY(EditAnywhere, Category = "Tutorial")
    FText TutorialTitleText;
    
    UPROPERTY(EditAnywhere, Category = "Tutorial")
    FText TutorialParragraphText;

    UPROPERTY(EditAnywhere, Category = "Tutorial")
    TSubclassOf<class UUserWidget> TutorialWidgetClass;

    bool bHasBeenActivated;
};
