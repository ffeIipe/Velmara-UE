#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MementoComponent.generated.h"

class AEntity;
class AWeapon;

USTRUCT(BlueprintType)
struct FEntityMementoState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    FName UniqueSaveID;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    TSubclassOf<AEntity> OwnerClass;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    bool bIsAlive;
    
    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    float Health;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    float Energy;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    TArray<TSubclassOf<AWeapon>> InventorySlots;

    UPROPERTY(VisibleAnywhere, Category = "Memento State")
    int32 ActiveSaveSlotIndex;
    
    FEntityMementoState()
        : UniqueSaveID(NAME_None)
        , OwnerClass(nullptr)
        , bIsAlive(false)
        , Health(100.0f)
        , Energy(10.f)
        , ActiveSaveSlotIndex(0)
    {
        Transform = FTransform::Identity;
        /*InventorySlots.Empty();*/
    }
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UMementoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMementoComponent();

    UFUNCTION(BlueprintPure, Category = "Memento")
    FEntityMementoState GetInternalSavedState() const { return InternalMementoState; }

    UFUNCTION(BlueprintCallable, Category = "Memento")
    void ApplyExternalState(const FEntityMementoState& StateToApply);

    FORCEINLINE FTransform GetLastSavedTransform() const { return InternalMementoState.Transform; }

    FEntityMementoState CaptureOwnerState();

    bool bShouldSaveInventory = true;
    
protected:
    virtual void BeginPlay() override;

    FEntityMementoState InternalMementoState;
};