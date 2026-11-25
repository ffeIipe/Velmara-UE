#include "GAS/VelmaraAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/World.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Core/BossEnemy.h"
#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/Damageable.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Player/CustomPlayerController.h"

UVelmaraAttributeSet::UVelmaraAttributeSet()
{
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

	if (const UAbilitySystemComponent* SourceASC = EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
	{
		const UVelmaraAttributeSet* SourceSet = SourceASC->GetSet<UVelmaraAttributeSet>();
		if (SourceSet && SourceSet != this)
		{
			FinalDamage += FMath::Max(SourceSet->GetBaseDamage(), 0.f);

			if (GetOwningAbilitySystemComponent()->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_BloodMarked))
			{
				FinalDamage *= BloodMarkDamageMultiplier;
			}
		}
	}

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

bool UVelmaraAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data)) return false;

	if (Data.EvaluatedData.Attribute == GetDamageIncomingAttribute())
	{
		const UAbilitySystemComponent* OwningASC = GetOwningAbilitySystemComponent();
		const AEntity* Victim = Cast<AEntity>(OwningASC ? OwningASC->GetAvatarActor() : nullptr);

		if (Victim)
		{
			if (!Victim->CanBeDamaged()) return false;

			UWorld* World = Victim->GetWorld();
			const ACustomPlayerController* PC = World ? Cast<ACustomPlayerController>(World->GetFirstPlayerController()) : nullptr;

			if (PC && PC->GetOriginalBody() == Victim && PC->IsPossessing()) return false;

			FGameplayTagContainer AllAssetTags;
			Data.EffectSpec.GetAllAssetTags(AllAssetTags);

			const FGameplayTag SunSetByCallerTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage.Sun"), false);
			const FGameplayTag SunDamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Environmental.Sun"), false);
			const bool bIsSunDamage =
				(SunSetByCallerTag.IsValid() && Data.EffectSpec.GetSetByCallerMagnitude(SunSetByCallerTag, false, 0.f) > 0.f) ||
				(SunDamageTypeTag.IsValid() && AllAssetTags.HasTag(SunDamageTypeTag));

			if (bIsSunDamage)
			{
				if (const USunlightExposureSubsystem* Exposure = World ? World->GetSubsystem<USunlightExposureSubsystem>() : nullptr)
				{
					if (Exposure->IsSunProtected(Victim) || Exposure->IsUnderSunGrace(Victim)) return false;
				}

				const FGameplayTag PossessedTag = FGameplayTag::RequestGameplayTag(FName("Character.Possessed"), false);
				if ((PC && PC->IsPossessedHost(Victim)) || (OwningASC && PossessedTag.IsValid() && OwningASC->HasMatchingGameplayTag(PossessedTag)))
				{
					return false;
				}
			}
		}

		const UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
		const AEntity* Attacker = SourceASC ? Cast<AEntity>(SourceASC->GetAvatarActor()) : nullptr;

		if (Attacker && Victim && Attacker != Victim && !Attacker->IsHostile(Victim)) return false;
	}

	return true;
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
            float DamageToShield = FMath::Min(CurrentShield, LocalDamageDone);
            float DamageToHealth = LocalDamageDone - DamageToShield;
            bool bHitByPossessedMinion = false;

            AEntity* DamagedEntity = Cast<AEntity>(GetOwningActor());
            ABossEnemy* BossVictim = Cast<ABossEnemy>(DamagedEntity);
            AActor* AttackerActor = nullptr;

            if (BossVictim)
            {
                const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();
                UAbilitySystemComponent* SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
                if (!SourceASC)
                {
                    SourceASC = Context.GetInstigatorAbilitySystemComponent();
                }
                AttackerActor = SourceASC ? SourceASC->GetAvatarActor() : Context.GetEffectCauser();
                const AActor* InstigatorActor = Context.GetInstigator();

                BossVictim->ResolveIncomingDamage(LocalDamageDone, CurrentShield, AttackerActor, InstigatorActor, SourceASC,
                    DamageToShield, DamageToHealth, bHitByPossessedMinion);
            }

            if (DamageToShield > 0.0f)
            {
                SetShield(FMath::Max(0.0f, CurrentShield - DamageToShield));
            }

            if (DamageToHealth > 0.0f)
            {
                const float CurrentHealth = GetHealth();
                SetHealth(FMath::Clamp(CurrentHealth - DamageToHealth, 0.0f, GetMaxHealth()));
            }

            if (BossVictim)
            {
                if (bHitByPossessedMinion)
                {
                    BossVictim->NotifyHitByPossessedMinion(AttackerActor, DamageToShield, DamageToHealth);
                }

                if (CurrentShield > 0.0f && GetShield() <= 0.0f)
                {
                    BossVictim->HandleSolarShieldBroken(AttackerActor);
                }
            }

            if (DamagedEntity)
            {
                if (GetHealth() <= 0.0f)
                {
                    DamagedEntity->EndJuggle();
                }
                else
                {
                    DamagedEntity->OnDamageReceived();
                }
            }

            const float EffectiveTotalDamage = DamageToShield + DamageToHealth;

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
                DeathPayload.InstigatorTags.AppendTags(DeathTagContainer);

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
                DamagePayload.EventMagnitude = EffectiveTotalDamage;
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
