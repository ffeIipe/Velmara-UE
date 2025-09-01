#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Item.h"
#include "Player/CharacterHumanStates.h"
#include "SpectralObject.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API ASpectralObject : public AItem
{
    GENERATED_BODY()

public:
    ASpectralObject();

   /* UFUNCTION()
    virtual void SetSpectralVisibility(bool bIsVisible);*/

    class USpectralObjectComponent* GetSpectralObjectComponent() { return SpectralObjectComponent; };

protected:
    void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    USpectralObjectComponent* SpectralObjectComponent;

    /*UMaterialInstanceDynamic* SpectralMaterial;

    UPROPERTY(EditInstanceOnly, Category = "Visibility")
    ECharacterForm VisibleTo;*/
};
