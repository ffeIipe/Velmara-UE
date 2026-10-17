#include "Features/BossSystem/Core/BossSystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Entities/Entity.h"

bool UBossSystemLibrary::IsValidBossTarget(const AActor* Boss, const AActor* Target, const bool bOnlyPlayerControlled)
{
	if (!IsValid(Target) || Target == Boss || Target->IsHidden()) return false;

	if (!UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target)) return false;

	const AEntity* TargetEntity = Cast<AEntity>(Target);
	if (TargetEntity && !TargetEntity->IsAlive()) return false;

	if (const APawn* Pawn = Cast<APawn>(Target); Pawn && Pawn->IsPlayerControlled()) return true;

	if (bOnlyPlayerControlled) return false;

	const AEntity* BossEntity = Cast<AEntity>(Boss);
	return BossEntity && TargetEntity && BossEntity->IsHostile(TargetEntity);
}

bool UBossSystemLibrary::IsImmuneToBossHit(const AActor* Target, const FGameplayTagContainer& ImmunityTags)
{
	if (ImmunityTags.IsEmpty()) return false;

	const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	return TargetASC && TargetASC->HasAnyMatchingGameplayTags(ImmunityTags);
}

bool UBossSystemLibrary::ApplyBossDamage(AActor* Instigator, AActor* DamageCauser, AActor* Target, const FBossDamageParams& Params, const float DamageMultiplier, const FHitResult& Hit)
{
	if (!Target || !Params.DamageEffectClass || !Params.SetByCallerTag.IsValid()) return false;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC) return false;

	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
	if (!SourceASC) SourceASC = TargetASC;

	const float FinalDamage = Params.Damage * DamageMultiplier;
	if (FinalDamage <= 0.f) return false;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(DamageCauser ? DamageCauser : Instigator);
	if (Hit.GetActor())
	{
		Context.AddHitResult(Hit, true);
	}

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(Params.DamageEffectClass, 1.f, Context);
	if (!SpecHandle.IsValid()) return false;

	if (Params.DamageTypeTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(Params.DamageTypeTag);
	}
	SpecHandle.Data->SetSetByCallerMagnitude(Params.SetByCallerTag, FinalDamage);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (Params.CueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = Hit.GetActor() ? FVector(Hit.ImpactPoint) : Target->GetActorLocation();
		CueParameters.Normal = Hit.GetActor() ? FVector(Hit.ImpactNormal) : FVector::UpVector;
		CueParameters.Instigator = Instigator;
		CueParameters.EffectCauser = DamageCauser;

		TargetASC->ExecuteGameplayCue(Params.CueTag, CueParameters);
	}

	return true;
}
