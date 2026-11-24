#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerMainHUD.generated.h"

class UInputPromptWidget;
class UInputPromptData;
class UPlayerMainWidget;
class UPaladinBossHealthBar;
class UVelmaraPrimaryGameLayout;
class UCommonActivatableWidget;

UCLASS()
class TESISUE_API APlayerMainHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	TSubclassOf<UPlayerMainWidget> PlayerMainWidgetClass;

public:
	UFUNCTION(BlueprintCallable)
	void TogglePlayerUI(bool Bool);

	UFUNCTION(BlueprintPure, Category = "Velmara|UI")
	UVelmaraPrimaryGameLayout* GetPrimaryLayout() const { return PrimaryLayout; }

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	UVelmaraPrimaryGameLayout* EnsurePrimaryLayout();

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Velmara|UI")
	void ShowNotification(const FText& Message, float DurationSeconds = 3.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	UPlayerMainWidget* PlayerMainWidgetInstance = nullptr;

private:
	UPROPERTY(Transient)
	TObjectPtr<UVelmaraPrimaryGameLayout> PrimaryLayout = nullptr;

	bool bAlreadySetted = false;
};
