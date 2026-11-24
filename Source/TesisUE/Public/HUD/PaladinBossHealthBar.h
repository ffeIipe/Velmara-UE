#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PaladinBossHealthBar.generated.h"

UCLASS()
class TESISUE_API UPaladinBossHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Boss|UI")
	void ReceivePercentagesFromBoss(float HealthHP, float ShieldHP);

	virtual void ReceivePercentagesFromBoss_Implementation(float HealthHP, float ShieldHP) {}

	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	virtual void SetBossDisplayName(const FText& InBossName) {}
};
