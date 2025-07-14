// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "PlayerHeroController.generated.h"

class UUserWidget;
class USoundMix;
class UWBP_OptionsMenu;

UCLASS()
class TESISUE_API APlayerHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	APlayerHeroController();
	
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void TogglePauseMenu();

	/*UFUNCTION(BlueprintCallable)
	void SetGenericTeamId(FGenericTeamId NewTeamID) override;*/

	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;

	UFUNCTION(BlueprintCallable)
	virtual FGenericTeamId GetGenericTeamId() const override;

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> OptionsMenuWidgetClass_Pause;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	USoundMix* PauseSoundMix;

private:
	UPROPERTY()
	UUserWidget* CurrentPauseMenuInstance;

	UPROPERTY()
	UUserWidget* CurrentOptionsMenuInstance_Pause;

	bool bIsGamePausedExplicitly;

	FGenericTeamId HeroTeamID;

public:
	UPROPERTY(BlueprintReadOnly)
	class APlayerMainHUD* PlayerMainHUD;

	void HandleBossHealth(float HealthHP, float ShieldHP);

	UFUNCTION(BlueprintCallable)
	void ToggleInput(bool Bool);
};
