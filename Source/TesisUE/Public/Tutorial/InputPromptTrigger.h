// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "InputPromptTrigger.generated.h"

class UPromptWidgetComponent;
class USphereComponent;

UCLASS()
class TESISUE_API AInputPromptTrigger : public ATrigger
{
	GENERATED_BODY()
	
public:
    AInputPromptTrigger();

protected:
	virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UPromptWidgetComponent* PromptWidgetComponent;
};
