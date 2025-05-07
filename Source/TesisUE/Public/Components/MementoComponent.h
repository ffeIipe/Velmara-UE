#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MementoComponent.generated.h"

USTRUCT(BlueprintType)
struct FEntityMementoState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    float Health;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    float Energy;

    // Constructor por defecto para inicializar valores
    FEntityMementoState()
        : Health(100.0f) // Valores por defecto, ajústalos según tu juego
        , Energy(100.0f)
    {
        Transform = FTransform::Identity;
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UMementoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMementoComponent();

    UFUNCTION(BlueprintCallable, Category = "Memento")
    void SaveState();

    UFUNCTION(BlueprintCallable, Category = "Memento")
    void LoadState();

    UFUNCTION(BlueprintPure, Category = "Memento")
    FEntityMementoState GetCurrentSavedState() const { return InternalMementoState; }

    UFUNCTION(BlueprintCallable, Category = "Memento")
    void ApplyExternalState(const FEntityMementoState& StateToApply);

    FORCEINLINE FTransform GetLastSavedTransform() { return InternalMementoState.Transform; }

protected:
    virtual void BeginPlay() override;

    FEntityMementoState InternalMementoState;

private:
    FEntityMementoState CaptureOwnerState() const;
};