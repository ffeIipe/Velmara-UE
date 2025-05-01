#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Player/CharacterStates.h"
#include "FormInterface.generated.h"

UINTERFACE(MinimalAPI)
class UFormInterface : public UInterface
{
	GENERATED_BODY()
};

class TESISUE_API IFormInterface
{
	GENERATED_BODY()

public:
	//UFUNCTION(BlueprintNativeEvent)
	//ECharacterForm GetCharacterForm();

	UFUNCTION(BlueprintNativeEvent)
	void PerformSpectralAttack();

	UFUNCTION(BlueprintNativeEvent)
	void PerformSpectralBarrier();
	
	UFUNCTION(BlueprintNativeEvent)
	void ResetSpectralAttack();
};
