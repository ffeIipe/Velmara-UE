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
	UPlayerMainWidget* PlayerMainWidget;

public:
	void SetPaladinBossHealthBar();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	UUserWidget* PaladinBossMainWidget;

	UPlayerMainWidget* PlayerMainWidgetInstance = nullptr;

	UPaladinBossHealthBar* PaladinBossHealthBarInstance = nullptr;

private:
	bool bAlreadySetted = false;
};