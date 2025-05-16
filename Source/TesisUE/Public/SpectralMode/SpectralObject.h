#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Item.h"
#include "Player/CharacterStates.h"
#include "SpectralObject.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API ASpectralObject : public AItem
{
    GENERATED_BODY()

public:
    ASpectralObject();

    UFUNCTION()
    virtual void SetSpectralVisibility(bool bIsVisible);

protected:
    virtual void BeginPlay() override;

    UMaterialInstanceDynamic* SpectralMaterial;

    UPROPERTY(EditInstanceOnly, Category = "Visibility")
    ECharacterForm VisibleTo;
};
