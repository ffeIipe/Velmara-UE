#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EntityData.generated.h"

class UInputAction;
class UNiagaraSystem;
class UCombatStrategy;

USTRUCT(BlueprintType)
struct FCombatData
{
    GENERATED_BODY()
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float BufferAttackDistance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curves")
    UCurveFloat* BufferCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curves")
    UCurveFloat* LaunchUpCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curves")
    UCurveFloat* BufferBackwardsCurve;
};

USTRUCT(BlueprintType)
struct FTargetingData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curves")
    UCurveFloat* SoftLockCurve;
};

USTRUCT(BlueprintType)
struct FMovementData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats ")
    float MaxRunSpeed = 800.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float MaxStrafeSpeed = 650.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
    float StunMaxWalkSpeed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Curve")
    UCurveFloat* DodgeCurve;
};

USTRUCT(BlueprintType)
struct FAttributeData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
    float MaxHealth = 100.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float Energy = 100.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float MaxEnergy = 100.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float DrainTickValue = 2.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float RegenerateTickValue = .5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shield")
    float MaxShieldHealth = 100.f;
};

USTRUCT(BlueprintType)
struct FInventoryData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    int32 MaxSlots = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    float InteractTraceLenght;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    float InteractTargetRadius;
};

USTRUCT(BlueprintType)
struct FPossessionData
{
    GENERATED_BODY()
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Possession")
    float ReleaseAndExecuteEnergyTax = 25.f;
};

UCLASS()
class TESISUE_API UEntityData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FCombatData CombatData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FMovementData MovementData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FAttributeData AttributeData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FInventoryData InventoryData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FPossessionData PossessionData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FTargetingData TargetingData;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strategy")
    TSubclassOf<UCombatStrategy> FirstModeStrategyClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strategy")
    TSubclassOf<UCombatStrategy> SecondModeStrategyClass;
};