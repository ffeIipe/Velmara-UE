#include "GAS/Abilities/SkeletonMageReanimateAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Components/CapsuleComponent.h"
#include "Features/ReanimationSystem/GraveSubsystem.h"
#include "GameFramework/Character.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace Reanimate
{
	void RiseFromGrave(UWorld* World, const FEntityGrave& Grave)
	{
		UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>();
		if (!Pool || !Grave.EntityClass) return;

		FVector Location = Grave.Location;

		// La posición de muerte puede estar en el aire (launch): se apoya en el navmesh y se sube media cápsula.
		if (const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			if (NavSystem->ProjectPointToNavigation(Location, NavLocation, FVector(200.f, 200.f, 500.f)))
			{
				Location = NavLocation.Location;
				if (const ACharacter* CharacterDefaults = Cast<ACharacter>(Grave.EntityClass->GetDefaultObject()))
				{
					Location.Z += CharacterDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
				}
			}
		}

		FRotator Rotation = FRotator::ZeroRotator;
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			Rotation.Yaw = (Player->GetActorLocation() - Location).Rotation().Yaw;
		}

		Pool->GetActorFromPool(Grave.EntityClass, FTransform(Rotation, Location));
	}
}

USkeletonMageReanimateAbility::USkeletonMageReanimateAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Reanimate"), false)));
}

bool USkeletonMageReanimateAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UGraveSubsystem* Graves = Avatar ? Avatar->GetWorld()->GetSubsystem<UGraveSubsystem>() : nullptr;
	return Graves && Graves->HasReadyGrave(Avatar->GetActorLocation(), SearchRadius, MaxGraveAge);
}

void USkeletonMageReanimateAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float Rate = GetAttackSpeedMultiplier();

	// Si el casteo se interrumpe (hit react, muerte) antes del evento, no se levanta nadie.
	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, CastEventDelay / Rate);
	DelayTask->OnFinish.AddDynamic(this, &USkeletonMageReanimateAbility::ReanimateGraves);
	DelayTask->ReadyForActivation();

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage, Rate);
		MontageTask->OnCompleted.AddDynamic(this, &USkeletonMageReanimateAbility::HandleMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &USkeletonMageReanimateAbility::HandleMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &USkeletonMageReanimateAbility::HandleMontageEnded);
		MontageTask->ReadyForActivation();
	}
}

void USkeletonMageReanimateAbility::ReanimateGraves()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	UGraveSubsystem* Graves = World ? World->GetSubsystem<UGraveSubsystem>() : nullptr;

	if (Graves)
	{
		for (const FEntityGrave& Grave : Graves->ClaimGraves(Avatar->GetActorLocation(), SearchRadius, MaxGraveAge, MaxRevivesPerCast))
		{
			if (GraveEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, GraveEffect, Grave.Location);
			}
			if (RiseSound)
			{
				UGameplayStatics::PlaySoundAtLocation(World, RiseSound, Grave.Location);
			}

			// Timer del mundo, no de la habilidad: una vez marcada la tumba el enemigo sale aunque maten al mago.
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(World, [World, Grave]()
			{
				Reanimate::RiseFromGrave(World, Grave);
			}), FMath::Max(RiseDelay, KINDA_SMALL_NUMBER), false);
		}
	}

	if (!CastMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USkeletonMageReanimateAbility::HandleMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
