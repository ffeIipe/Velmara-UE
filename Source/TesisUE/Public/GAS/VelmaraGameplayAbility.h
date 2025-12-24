#pragma once

#include "CoreMinimal.h"
#include "VelmaraAbilityInputID.h"
#include "Abilities/GameplayAbility.h"
#include "VelmaraGameplayAbility.generated.h"

enum class EVelmaraAbilityInputID : uint8;

UCLASS()
class TESISUE_API UVelmaraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UVelmaraGameplayAbility();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	EVelmaraAbilityInputID AbilityInputID = EVelmaraAbilityInputID::None;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bActivateOnGiven = false;

protected:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
};
