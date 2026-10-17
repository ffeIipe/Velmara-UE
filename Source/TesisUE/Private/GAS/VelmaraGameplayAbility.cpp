#include "GAS/VelmaraGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "GAS/VelmaraAttributeSet.h"

UVelmaraGameplayAbility::UVelmaraGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UVelmaraGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateOnGiven)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

float UVelmaraGameplayAbility::GetAttackSpeedMultiplier() const
{
	if (const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo())
	{
		bool bFound = false;
		const float AttackSpeed = AbilitySystem->GetGameplayAttributeValue(UVelmaraAttributeSet::GetAttackSpeedAttribute(), bFound);

		if (bFound && AttackSpeed > 0.f) return AttackSpeed;
	}
	return 1.f;
}

#if WITH_EDITOR
void UVelmaraGameplayAbility::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UGameplayAbility, AbilityTags))
	{
		const UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();

		FGameplayTagContainer CurrentTags = AbilityTags;

		for (const FGameplayTag& Tag : CurrentTags)
		{
			FGameplayTagContainer ParentTags = TagManager.RequestGameplayTagParents(Tag);

			AbilityTags.AppendTags(ParentTags);
		}
	}
}
#endif
