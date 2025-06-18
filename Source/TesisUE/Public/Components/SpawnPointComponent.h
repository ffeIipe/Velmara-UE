#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SpawnPointComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpawnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	USpawnPointComponent();
};
