// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputPromptTrigger.generated.h"

class UPromptWidgetComponent;
class USphereComponent;

UCLASS()
class TESISUE_API AInputPromptTrigger : public AActor
{
	GENERATED_BODY()
	
public:
    AInputPromptTrigger();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UPROPERTY(VisibleAnywhere)
    USphereComponent* TriggerArea;

    UPROPERTY(VisibleAnywhere)
    UPromptWidgetComponent* PromptWidgetComponent;
};
