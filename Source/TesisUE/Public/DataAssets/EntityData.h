#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EntityData.generated.h"

class UVelmaraGameplayAbility;
class UInputAction;
class UNiagaraSystem;

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

//USTRUCT(BlueprintType)
//struct FMovementData
//{
//    GENERATED_BODY()
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats ")
//    float MaxRunSpeed = 800.f;
//    
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
//    float MaxStrafeSpeed = 650.f;
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
//    float StunMaxWalkSpeed;
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
//    UCurveFloat* DodgeCurve;
//};

USTRUCT(BlueprintType)
struct FAttributeData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float DrainTickValue = 2.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
    float RegenerateTickValue = .5f;
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
struct FSpringArmData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpringArm")
    float SpringArmLength = 300.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpringArm")
    FVector SocketOffset = FVector(0, 90.f, 60.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpringArm")
    bool CameraLag = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpringArm")
    float CameraLagSpeed = 8.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpringArm")
    float CameraRotationLagSpeed = 30.f;
};

USTRUCT(BlueprintType)
struct FCharMoveStats
{
    GENERATED_BODY()

    // Character Movement (General Settings)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
    float GravityScale = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General Settings")
    float MaxAcceleration = 2000.0f;

    // Advanced
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    int32 MaxJumpApexAttemptsPerSimulation = 1;

    // Character Movement: Walking
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walking")
    float MaxWalkSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walking")
    float BrakingDecelerationWalking = 2000.0f;

    // Character Movement: Jumping / Falling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping / Falling")
    float JumpZVelocity = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping / Falling")
    float AirControl = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jumping / Falling")
    float AirControlBoostMultiplier = 1.0f;

    // Advanced
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    float JumpOffJumpZFactor = 0.25f;

    // Character Movement (Rotation Settings)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation Settings")
    FRotator RotationRate = FRotator(0.0f, 400.0f, 0.0f);

    // Root Motion
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
    float FormerBaseVelocityDecayHalfLife = 1.0f;
};

USTRUCT(BlueprintType)
struct FCharacterModeConfig
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    int32 TeamID;

    UPROPERTY(EditDefaultsOnly)
    bool bShowWeapon;
    
    UPROPERTY(EditDefaultsOnly)
    bool bUseControllerRotationYaw;
};

UCLASS()
class TESISUE_API UEntityData : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    TArray<TSubclassOf<UVelmaraGameplayAbility>> DefaultAbilities;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FCombatData CombatData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FInventoryData InventoryData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FTargetingData TargetingData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FCharMoveStats CharMoveData;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EntityData")
    FSpringArmData SpringArmData;

    /**
      This property is optional, so if you are filling these stats and do not
      want to spawn with a weapon, leave it blank.
     **/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InitialWeapon")
    TSubclassOf<AActor> InitialWeapon;
};