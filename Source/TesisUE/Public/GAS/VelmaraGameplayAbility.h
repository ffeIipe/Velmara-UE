#pragma once

#include "CoreMinimal.h"
#include "VelmaraAbilityInputID.h"
#include "Abilities/GameplayAbility.h"
#include "VelmaraGameplayAbility.generated.h"

enum class EVelmaraAbilityInputID : uint8;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class TESISUE_API UVelmaraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UVelmaraGameplayAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	EVelmaraAbilityInputID AbilityInputID = EVelmaraAbilityInputID::None;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bActivateOnGiven = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool bReplacesSharedInput = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player", meta = (ClampMin = "0"))
	float PlayerCooldown = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player", meta = (ClampMin = "0"))
	float PlayerEnergyCost = 0.f;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetAttackSpeedMultiplier() const;

protected:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	bool IsPlayerControlledAvatar(const FGameplayAbilityActorInfo* ActorInfo) const;

	UAbilitySystemComponent* GetPlayerEnergyASC(const FGameplayAbilityActorInfo* ActorInfo) const;

	mutable double NextPlayerCastTime = 0.0;

#if WITH_EDITOR
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
