#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PaladinBossHealthBar.generated.h"

UCLASS()
class TESISUE_API UPaladinBossHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void ReceivePercentagesFromBoss(float HealthHP, float ShieldHP);
};
