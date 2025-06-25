#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Tutorial/InputPromptData.h"
#include "PlayerMainHUD.generated.h"

class UInputPromptWidget;
class UInputPromptData;
class UPlayerMainWidget;
class UPaladinBossHealthBar;

UCLASS()
class TESISUE_API APlayerMainHUD : public AHUD
{
	GENERATED_BODY()
	
protected:
    virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	TSubclassOf<UPlayerMainWidget> PlayerMainWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paladin")
	TSubclassOf<UUserWidget> PaladinUIWidgetClass;

public:
	void SetPaladinBossHealthBar();

	void TogglePaladinUI(bool Bool);

	UFUNCTION(BlueprintCallable)
	void TogglePlayerUI(bool Bool);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	TSubclassOf<UUserWidget> PaladinBossMainWidgetClass;

	UPlayerMainWidget* PlayerMainWidgetInstance = nullptr;

	UPaladinBossHealthBar* PaladinBossHealthBarInstance = nullptr;

	UUserWidget* PaladinUIInstance = nullptr;

private:
	bool bAlreadySetted = false;
};