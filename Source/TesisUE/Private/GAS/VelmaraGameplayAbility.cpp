#include "GAS/VelmaraGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GAS/VelmaraAttributeSet.h"
#include "Player/CustomPlayerController.h"

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

bool UVelmaraGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	if (bReplacesSharedInput || AbilityInputID == EVelmaraAbilityInputID::None || !IsPlayerControlledAvatar(ActorInfo)) return true;

	const UAbilitySystemComponent* AbilitySystem = ActorInfo->AbilitySystemComponent.Get();
	if (!AbilitySystem) return true;

	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		const UVelmaraGameplayAbility* Other = Cast<UVelmaraGameplayAbility>(Spec.Ability);
		if (Other && Other->bReplacesSharedInput && Spec.InputID == static_cast<int32>(AbilityInputID)) return false;
	}
	return true;
}

bool UVelmaraGameplayAbility::IsPlayerControlledAvatar(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const APawn* Pawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Pawn && Pawn->IsPlayerControlled();
}

UAbilitySystemComponent* UVelmaraGameplayAbility::GetPlayerEnergyASC(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const APawn* Pawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}

	if (const ACustomPlayerController* PlayerController = Cast<ACustomPlayerController>(Pawn->GetController()))
	{
		if (PlayerController->IsPossessing() && PlayerController->GetOriginalBody())
		{
			if (UAbilitySystemComponent* BodyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerController->GetOriginalBody()))
			{
				return BodyASC;
			}
		}
	}

	return ActorInfo->AbilitySystemComponent.Get();
}

bool UVelmaraGameplayAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (IsPlayerControlledAvatar(ActorInfo) && PlayerCooldown > 0.f)
	{
		const UWorld* World = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr;
		if (World && World->GetTimeSeconds() < NextPlayerCastTime)
		{
			return false;
		}
	}

	return true;
}

void UVelmaraGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	if (IsPlayerControlledAvatar(ActorInfo) && PlayerCooldown > 0.f)
	{
		if (const UWorld* World = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
		{
			NextPlayerCastTime = World->GetTimeSeconds() + PlayerCooldown;
		}
	}
}

float UVelmaraGameplayAbility::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	float Remaining = Super::GetCooldownTimeRemaining(ActorInfo);

	if (IsPlayerControlledAvatar(ActorInfo) && PlayerCooldown > 0.f)
	{
		if (const UWorld* World = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
		{
			Remaining = FMath::Max(Remaining, static_cast<float>(NextPlayerCastTime - World->GetTimeSeconds()));
		}
	}

	return FMath::Max(0.f, Remaining);
}

bool UVelmaraGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (IsPlayerControlledAvatar(ActorInfo) && PlayerEnergyCost > 0.f)
	{
		const UAbilitySystemComponent* EnergyASC = GetPlayerEnergyASC(ActorInfo);
		const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
		if (!EnergyASC || !EnergyASC->HasAttributeSetForAttribute(EnergyAttribute))
		{
			return false;
		}
		if (EnergyASC->GetNumericAttribute(EnergyAttribute) < PlayerEnergyCost)
		{
			return false;
		}
	}

	return true;
}

void UVelmaraGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (IsPlayerControlledAvatar(ActorInfo) && PlayerEnergyCost > 0.f)
	{
		if (UAbilitySystemComponent* EnergyASC = GetPlayerEnergyASC(ActorInfo))
		{
			const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
			if (EnergyASC->HasAttributeSetForAttribute(EnergyAttribute))
			{
				const float CurrentEnergy = EnergyASC->GetNumericAttribute(EnergyAttribute);
				EnergyASC->SetNumericAttributeBase(EnergyAttribute, FMath::Max(0.f, CurrentEnergy - PlayerEnergyCost));
			}
		}
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
