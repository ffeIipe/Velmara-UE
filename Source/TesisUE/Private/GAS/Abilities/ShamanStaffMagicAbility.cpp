#include "GAS/Abilities/ShamanStaffMagicAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Features/BossSystem/Actors/BossProjectile.h"
#include "GameplayEffect.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

UShamanStaffMagicAbility::UShamanStaffMagicAbility()
{
	ProjectileClass = ABossProjectile::StaticClass();

	// Mismos defaults que el daño del boss (BossEnemy / BossCombatComponent).
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GAS/Effects/GE_Damage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageParams.DamageEffectClass = DamageEffectFinder.Class;
	}
	DamageParams.SetByCallerTag = FGameplayTag::RequestGameplayTag(FName("Damage"), false);
	DamageParams.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false);
	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(FName("State.Dodging"), false); DodgeTag.IsValid())
	{
		DamageParams.ImmunityTags.AddTag(DodgeTag);
	}
}

void UShamanStaffMagicAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float Rate = GetAttackSpeedMultiplier();

	// Si el montage se interrumpe (hit react, muerte) la habilidad termina y este delay se cancela: no sale el proyectil.
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FireDelay / Rate);
	DelayTask->OnFinish.AddDynamic(this, &UShamanStaffMagicAbility::FireProjectile);
	DelayTask->ReadyForActivation();

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage, Rate);
		MontageTask->OnCompleted.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->ReadyForActivation();
	}
}

void UShamanStaffMagicAbility::FireProjectile()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UActorPoolSubsystem* Pool = Avatar ? Avatar->GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;

	if (Pool && ProjectileClass)
	{
		const FVector SpawnLocation = GetStaffTipLocation(Avatar);

		FVector Direction = Avatar->GetActorForwardVector();
		if (const AActor* Target = FindTarget())
		{
			Direction = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
		}

		if (ABossProjectile* Projectile = Cast<ABossProjectile>(Pool->GetActorFromPool(ProjectileClass, FTransform(Direction.Rotation(), SpawnLocation))))
		{
			Projectile->Launch(Avatar, Direction, ProjectileSpeed, ProjectileLifetime, DamageParams);
		}
	}

	// Sin montage no hay nada más que esperar.
	if (!CastMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShamanStaffMagicAbility::HandleMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

AActor* UShamanStaffMagicAbility::FindTarget() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (const UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(const_cast<AActor*>(Avatar)))
	{
		if (AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))))
		{
			return Target;
		}
	}
	return UGameplayStatics::GetPlayerPawn(Avatar, 0);
}

FVector UShamanStaffMagicAbility::GetStaffTipLocation(const AActor* Avatar) const
{
	TInlineComponentArray<USceneComponent*> Components(Avatar);
	for (const USceneComponent* Component : Components)
	{
		if (Component->DoesSocketExist(StaffTipSocket))
		{
			return Component->GetSocketLocation(StaffTipSocket);
		}
	}
	return Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.f;
}
