#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "EntityData.generated.h"

class UVelmaraGameplayAbility;
class UGameplayEffect;
class UInputAction;
class UNiagaraSystem;

UENUM(BlueprintType)
enum EReactionType : uint8
{
    None,
    Directional,
    PushOff,
    Launch,
    StayInAir,
    Crasher
};

USTRUCT(BlueprintType)
struct FInventoryData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 MaxSlots = 2;

    /** This property is optional, so if you are filling these stats and do not
      want to spawn with a weapon, leave it in blank. **/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<AActor> InitialWeapon;
};

USTRUCT(BlueprintType)
struct FSpringArmData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float SpringArmLength = 300.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector SocketOffset = FVector(0, 90.f, 60.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool CameraLag = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float CameraLagSpeed = 8.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float CameraRotationLagSpeed = 30.f;
};

USTRUCT(BlueprintType)
struct FCharMoveStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GravityScale = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxAcceleration = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxJumpApexAttemptsPerSimulation = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxWalkSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BrakingDecelerationWalking = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float JumpZVelocity = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AirControl = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AirControlBoostMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float JumpOffJumpZFactor = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator RotationRate = FRotator(0.0f, 400.0f, 0.0f);
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

USTRUCT(BlueprintType)
struct FDeathAnimDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag DeathTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName DeathAnimSection = NAME_Default;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bUseRagdoll = false;
};

USTRUCT(BlueprintType)
struct FHitReactDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag DamageTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName HitReactAnimSection = NAME_Default;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TEnumAsByte<EReactionType> ReactionType = None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float MagnitudeReaction = 0.f;
};

UCLASS()
class TESISUE_API UEntityData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Instanced, Category = "Entity|Abilities")
    TArray<UVelmaraGameplayAbility*> DefaultAbilities;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Inventory")
    FInventoryData InventoryData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|CharacterMovement")
    FCharMoveStats CharMoveData;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Entity|SpringArm")
    FSpringArmData SpringArmData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Animations|HitReact")
    TArray<FHitReactDefinition> HitReactsDefinitions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Animations|HitReact")
    UAnimMontage* HitReactMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Animations|Death")
    TArray<FDeathAnimDefinition> DeathDefinitions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Animations|Death")
    UAnimMontage* DeathMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Unarmed")
    FGameplayTag UnarmedDamageTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Unarmed")
    TSubclassOf<UGameplayEffect> DamageEffectSpecClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Unarmed")
    FGameplayTag DefaultUnarmedGameplayCueTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Entity|Unarmed")
    float UnarmedDamage;
};