#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyEditorLibrary.generated.h"

class UAnimMontage;

UCLASS()
class TESISUE_API UEnemyEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy|Editor")
	static bool CopyAttackNotifiesFromTemplate(UAnimMontage* Montage, UAnimMontage* Template, float HitStart, float HitEnd, float ComboWindowEnd);
};
