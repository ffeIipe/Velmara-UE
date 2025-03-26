#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
    
    UPROPERTY(VisibleAnywhere)
    UBoxComponent* BoxCollider;

    UMaterialInstanceDynamic* SpectralMaterial;
};
