#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EntityData.generated.h"

USTRUCT(BlueprintType)
struct FCombatData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoftLock")
    float SoftLockDistance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoftLock")
    float SoftLockRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoftLock")
    float TrackTargetRadius;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoftLock")
    UCurveFloat* SoftLockCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buffer")
    float BufferAttackDistance;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buffer")
    UCurveFloat* BufferCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | LightAttack")
    TArray<UAnimMontage*> LightAttackCombo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | JumpAttack")
    TArray<UAnimMontage*> JumpAttackCombo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | HeavyAttack")
    TArray<UAnimMontage*> HeavyAttackCombo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    UAnimMontage* BlockMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    UAnimMontage* FinisherMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    UAnimMontage* CrasherMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    UAnimMontage* LaunchMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
    UAnimMontage* HitReactMontage;
};

USTRUCT(BlueprintType)
struct FMovementData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats | Movement")
    float MaxRunSpeed = 800.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats | Movement")
    float MaxStrafeSpeed = 650.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats | Dodge")
    float BufferDodgeDistance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats | Dodge")
    UCurveFloat* DodgeCurve;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats | DoubleJump")
    float LaunchStrength = 800.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | Dodge")
    UAnimMontage* DodgeMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | Jump")
    UAnimMontage* JumpMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages | DoubleJump")
    UAnimMontage* DoubleJumpMontage;
};

USTRUCT(BlueprintType)
struct FAttributeData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Properties | Health")
    float MaxHealth = 100.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Properties | Energy")
    float Energy = 100.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Properties | Energy")
    float DrainTickValue = 2.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Properties | Energy")
    float RegenerateTickValue = .5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Properties | Shield")
    float MaxShieldHealth = 100.f;
};

USTRUCT(BlueprintType)
struct FInventoryData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    int32 MaxSlots = 2;
};

USTRUCT(BlueprintType)
struct FPossessionData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Possession")
    float PossessDistance = 1500.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Possession")
    float PossessRadius = 50.f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Possession")
    float ReleaseAndExecuteEnergyTax = 25.f;
};

UCLASS()
class TESISUE_API UEntityData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity Data | Combat")
    FCombatData CombatData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity Data | Movement")
    FMovementData MovementData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity Data | Attributes")
    FAttributeData AttributeData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity Data | Inventory")
    FInventoryData InventoryData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity Data | Possession")
    FPossessionData PossessionData;
};