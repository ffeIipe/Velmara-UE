#include "GAS/VelmaraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Entities/Entity.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/Damageable.h"
#include "GAS/VelmaraGameplayTags.h"

UVelmaraAttributeSet::UVelmaraAttributeSet()
{
	// Valores neutros para que las entidades que no los configuran (enemigos) no se vean afectadas.
	InitArmor(0.f);
	InitBaseDamage(0.f);
	InitAttackSpeed(1.f);
}

void UVelmaraAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.1f);
	}
}

float UVelmaraAttributeSet::CalculateFinalDamage(const float RawDamage, const FGameplayEffectSpec& EffectSpec) const
{
	if (RawDamage <= 0.f) return RawDamage;

	float FinalDamage = RawDamage;

	// Daño base del atacante (no se aplica al daño auto-infligido, ej. el sol).
	if (const UAbilitySystemComponent* SourceASC = EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
	{
		const UVelmaraAttributeSet* SourceSet = SourceASC->GetSet<UVelmaraAttributeSet>();
		if (SourceSet && SourceSet != this)
		{
			FinalDamage += FMath::Max(SourceSet->GetBaseDamage(), 0.f);

			// Marca de sangre del pulso vampiro.
			if (GetOwningAbilitySystemComponent()->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_BloodMarked))
			{
				FinalDamage *= BloodMarkDamageMultiplier;
			}
		}
	}

	// Armadura del objetivo.
	const float CurrentArmor = FMath::Max(GetArmor(), 0.f);
	FinalDamage *= ArmorMitigationConstant / (ArmorMitigationConstant + CurrentArmor);

	return FinalDamage;
}

void UVelmaraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UVelmaraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    
    if (Data.EvaluatedData.Attribute == GetDamageIncomingAttribute())
    {
        const float LocalDamageDone = CalculateFinalDamage(GetDamageIncoming(), Data.EffectSpec);

        SetDamageIncoming(0.0f);

        if (LocalDamageDone > 0.0f)
        {
            const bool bWasAlive = GetHealth() > 0.0f;

            const float CurrentShield = GetShield();
            const float DamageToShield = FMath::Min(CurrentShield, LocalDamageDone);

            if (DamageToShield > 0.0f)
            {
                SetShield(CurrentShield - DamageToShield);
            }

            const float DamageToHealth = LocalDamageDone - DamageToShield;
            if (DamageToHealth > 0.0f)
            {
                const float CurrentHealth = GetHealth();
                SetHealth(FMath::Clamp(CurrentHealth - DamageToHealth, 0.0f, GetMaxHealth()));
            }

            if (GetHealth() <= 0.0f)
            {
                FGameplayTagContainer DeathTagContainer;
                Data.EffectSpec.GetAllAssetTags(DeathTagContainer);

                FGameplayTag DeathReason = FGameplayTag::RequestGameplayTag(FName("Death.Default"));

                FGameplayEventData DeathPayload;
                DeathPayload.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Death"));
                DeathPayload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
                DeathPayload.Target = GetOwningActor();
                DeathPayload.InstigatorTags.AddTag(DeathReason);

                // Solo en el golpe que la mata: los golpes posteriores también llegan acá con la vida en 0.
                if (bWasAlive)
                {
                    if (AEntity* OwnerEntity = Cast<AEntity>(GetOwningActor()))
                    {
                        OwnerEntity->NotifyDeathToWorld();
                    }
                }

                if (const TScriptInterface<IDamageable> Damageable = GetOwningActor())
                {
                    Damageable->Execute_MortalDamage(GetOwningActor(), DeathPayload);
                }
            }
            else
            {
                FGameplayEventData DamagePayload;
                DamagePayload.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Hit"));
                DamagePayload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
                DamagePayload.EventMagnitude = LocalDamageDone; 
                DamagePayload.Target = GetOwningActor();

                Data.EffectSpec.GetAllAssetTags(DamagePayload.InstigatorTags);
              
                if (const FHitResult* HitResult = Data.EffectSpec.GetContext().GetHitResult())
                {
                    DamagePayload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(*HitResult);
                }

                if (const TScriptInterface<IDamageable> Damageable = GetOwningActor())
                {
                    Damageable->Execute_ReceiveDamage(GetOwningActor(), DamagePayload);
                }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
    	float CurrentMax = GetMaxHealth();
    	if (CurrentMax > 0.0f)
    	{
    		SetHealth(FMath::Clamp(GetHealth(), 0.0f, CurrentMax));
    	}
    }
    else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
    {
    	float CurrentMax = GetMaxShield();
    	if (CurrentMax > 0.0f)
    	{
    		SetShield(FMath::Clamp(GetShield(), 0.0f, CurrentMax));
    	}
    }
    else if (Data.EvaluatedData.Attribute == GetEnergyAttribute())
    {
    	float CurrentMax = GetMaxEnergy();
    	if (CurrentMax > 0.0f)
    	{
    		SetEnergy(FMath::Clamp(GetEnergy(), 0.0f, CurrentMax));
    	}
    }
    else if (Data.EvaluatedData.Attribute == GetMovementSpeedAttribute())
    {
	    
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
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, Armor, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, BaseDamage, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UVelmaraAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always)
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

void UVelmaraAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, MovementSpeed, OldMovementSpeed)
}

void UVelmaraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, Armor, OldArmor)
}

void UVelmaraAttributeSet::OnRep_BaseDamage(const FGameplayAttributeData& OldBaseDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, BaseDamage, OldBaseDamage)
}

void UVelmaraAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UVelmaraAttributeSet, AttackSpeed, OldAttackSpeed)
}
