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

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* BoxCollider;

    UMaterialInstanceDynamic* SpectralMaterial;

private:
    UPROPERTY(EditInstanceOnly, Category = "Visibility")
    ECharacterForm VisibleTo;
    
};
