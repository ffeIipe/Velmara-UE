// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Tutorial/InputPromptData.h"
#include "PlayerMainHUD.generated.h"

class UInputPromptWidget;
class UInputPromptData;

UCLASS()
class TESISUE_API APlayerMainHUD : public AHUD
{
	GENERATED_BODY()
	
protected:
    virtual void BeginPlay() override;
};
