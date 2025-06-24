#pragma once

#include "CoreMinimal.h"
//#include "GameFramework/Actor.h"
#include "SceneEvents/Trigger.h"
#include "TutorialTrigger.generated.h"

UCLASS()
class TESISUE_API ATutorialTrigger : public ATrigger
{
	GENERATED_BODY()

public:
    ATutorialTrigger();

protected:
    void BeginPlay() override;

    //void OnSphereBeginOverlap(
    //    UPrimitiveComponent* OverlappedComponent,
    //    AActor* OtherActor,
    //    UPrimitiveComponent* OtherComp,
    //    int32 OtherBodyIndex, bool bFromSweep,
    //    const FHitResult& SweepResult) override;

    UFUNCTION()
    void EnableTutorial();

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
