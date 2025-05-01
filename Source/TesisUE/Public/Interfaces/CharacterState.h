#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Components/CharacterStateComponent.h"
#include "CharacterState.generated.h"

UINTERFACE(MinimalAPI)
class UCharacterState : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API ICharacterState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	UCharacterStateComponent* GetCharacterStateComponent();
};
