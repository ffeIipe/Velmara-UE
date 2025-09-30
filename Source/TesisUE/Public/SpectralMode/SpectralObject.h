#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Spectral.h"
#include "Items/Item.h"
#include "SpectralObject.generated.h"

enum class ECharacterModeStates : uint8;

UCLASS()
class TESISUE_API ASpectralObject : public AItem, public ISpectral
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    virtual void AddToSpectralObjects() override;
    virtual void ActivateVisiblity() override;
    virtual void DeactivateVisibility() override;
};
