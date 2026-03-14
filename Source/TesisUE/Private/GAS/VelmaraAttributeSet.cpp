#include "GAS/VelmaraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Entities/Entity.h"
#include "GameFramework/CharacterMovementComponent.h"

UVelmaraAttributeSet::UVelmaraAttributeSet() { }

void UVelmaraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		Health = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}

	if (Attribute == GetMovementSpeedMultiplierAttribute())
	{
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetOwningActor()))
		{
			if (UCharacterMovementComponent* CharMovComp = TargetCharacter->GetCharacterMovement())
			{
				CharMovComp->MaxWalkSpeed = NewValue;
			}
		}
	}
}

void UVelmaraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, GetOwningActor()->GetName());
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (GetHealth() <= 0.0f)
		{
			FGameplayTagContainer DeathTagContainer;
			Data.EffectSpec.GetAllAssetTags(DeathTagContainer);

			FGameplayTag DeathReason = FGameplayTag::RequestGameplayTag(FName("Death.Default"));

			for (const FGameplayTag& Tag : DeathTagContainer)
			{
				if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Death"))))
				{
					DeathReason = Tag;
					break;
				}
                    
				if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Damage"))) && 
					DeathReason == FGameplayTag::RequestGameplayTag(FName("Death.Default")))
				{
					DeathReason = Tag;
				}
			}
				
			FGameplayEventData DeathPayload;
			DeathPayload.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Death"));
			DeathPayload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
			DeathPayload.Target = GetOwningActor();
			DeathPayload.InstigatorTags.AddTag(DeathReason);
		
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			GetOwningActor(),
			DeathPayload.EventTag,
			DeathPayload
			);

			//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "Death Entity...");
			
			if (OnZeroHealth.IsBound()) OnZeroHealth.Broadcast();
		}
		else
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "Damage received...");
			
			FGameplayEventData DamagePayload;
			DamagePayload.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Hit"));
			DamagePayload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
			DamagePayload.EventMagnitude = -Data.EvaluatedData.Magnitude;
			DamagePayload.Target = GetOwningActor();

			Data.EffectSpec.GetAllAssetTags(DamagePayload.InstigatorTags);
			
			if (const FHitResult* HitResult = Data.EffectSpec.GetContext().GetHitResult())
			{
				DamagePayload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(*HitResult);
			}

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				GetOwningActor(), 
				DamagePayload.EventTag, 
				DamagePayload
			);
		}
	}
}

void UVelmaraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, Health, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, Shield, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, MaxShield, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, Energy, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, MovementSpeedMultiplier, COND_None, REPNOTIFY_Always)
}

void UVelmaraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, Health, OldHealth);
}

void UVelmaraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, MaxHealth, OldMaxHealth)
}

void UVelmaraAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, Shield, OldShield)
}

void UVelmaraAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, MaxShield, OldMaxShield)
}

void UVelmaraAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldEnergy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, Energy, OldEnergy)
}

void UVelmaraAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldMaxEnergy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, MaxEnergy, OldMaxEnergy)
}

void UVelmaraAttributeSet::OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldMovementSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, MovementSpeedMultiplier, OldMovementSpeed)
}




