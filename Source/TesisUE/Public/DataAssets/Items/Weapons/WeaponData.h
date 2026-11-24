#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"
#include "WeaponData.generated.h"

class UGameplayAbility;
class UTexture2D;
class UCurveFloat;
class UAnimSequence;

USTRUCT(BlueprintType)
struct FWeaponKnockback
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", Units = "cm"))
	float Distance = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.02", Units = "s"))
	float Duration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> ProgressCurve = nullptr;
};

USTRUCT(BlueprintType)
struct FWeaponOverlaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimSequence> OverlayPose = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	float OverlayPoseTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float SpineWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float HeadWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float ArmLeftWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float ArmRightWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float LeftHandIKWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1"))
	float IdleLegsWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LeftHandIKSocket = "LeftHandIK";
};

UCLASS()
class TESISUE_API UWeaponData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTag WeaponTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	FGameplayTag HolsterTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float BaseDamage = -10.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float CriticalChance = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float CriticalDamageMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knockback")
	FWeaponKnockback Knockback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	FName CustomInSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TSubclassOf<UAnimInstance> AnimLayer = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Overlay")
	FWeaponOverlaySettings Overlay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (TitleProperty = "DisplayName"))
	TArray<FWeaponComboUpgrade> ComboUpgrades;
};