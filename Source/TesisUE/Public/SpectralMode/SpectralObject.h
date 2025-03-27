#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/CharacterStates.h"
#include "SpectralObject.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API ASpectralObject : public AActor
{
    GENERATED_BODY()

public:
    ASpectralObject();

    UFUNCTION()
    void SetSpectralVisibility(bool bIsVisible);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditInstanceOnly, Category = "Visibility")
    EPlayerForm VisibleTo;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
    
    UPROPERTY(VisibleAnywhere)
    UBoxComponent* BoxCollider;

    UMaterialInstanceDynamic* SpectralMaterial;
};
