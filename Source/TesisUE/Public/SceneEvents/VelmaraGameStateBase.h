#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VelmaraGameStateBase.generated.h"

class AItem;

UCLASS()
class TESISUE_API AVelmaraGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:

protected:
	bool bIsLoadingFromSave = false;
};