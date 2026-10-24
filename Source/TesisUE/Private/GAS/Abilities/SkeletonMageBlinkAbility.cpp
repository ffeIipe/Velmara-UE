#include "GAS/Abilities/SkeletonMageBlinkAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"

namespace Blink
{
	constexpr int32 MaxPlacementAttempts = 16;
}

USkeletonMageBlinkAbility::USkeletonMageBlinkAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Blink"), false)));

	// Mientras está deshecho no lo tocan los golpes que respetan el dodge.
	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(FName("State.Dodging"), false); DodgeTag.IsValid())
	{
		ActivationOwnedTags.AddTag(DodgeTag);
	}
}

bool USkeletonMageBlinkAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AActor* Target = Avatar ? FindTarget(Avatar) : nullptr;
	return Target && FVector::Dist(Avatar->GetActorLocation(), Target->GetActorLocation()) <= TriggerRange;
}

void USkeletonMageBlinkAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	AActor* Target = Avatar ? FindTarget(Avatar) : nullptr;

	// Sin lugar a donde ir no se gasta el cooldown.
	if (!Target || !FindDestination(Avatar, Target, Destination) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BlinkTarget = Target;
	SetVanished(true);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(VanishDuration, KINDA_SMALL_NUMBER));
	DelayTask->OnFinish.AddDynamic(this, &USkeletonMageBlinkAbility::Reappear);
	DelayTask->ReadyForActivation();
}

void USkeletonMageBlinkAbility::Reappear()
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		FRotator Rotation(0.f, Avatar->GetActorRotation().Yaw, 0.f);
		if (const AActor* Target = BlinkTarget.Get())
		{
			Rotation.Yaw = (Target->GetActorLocation() - Destination).Rotation().Yaw;
		}
		Avatar->TeleportTo(Destination, Rotation);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USkeletonMageBlinkAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	// Pase lo que pase (cancelada, muerte), el mago no puede quedar invisible.
	if (bVanished)
	{
		SetVanished(false);
	}
	BlinkTarget.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkeletonMageBlinkAbility::SetVanished(const bool bVanish)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	bVanished = bVanish;

	if (UNiagaraSystem* Effect = bVanish ? VanishEffect : AppearEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Avatar, Effect, Avatar->GetActorLocation());
	}
	if (BlinkSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Avatar, BlinkSound, Avatar->GetActorLocation());
	}

	Avatar->SetActorHiddenInGame(bVanish);
	Avatar->SetActorEnableCollision(!bVanish);

	if (const ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
}

AActor* USkeletonMageBlinkAbility::FindTarget(const AActor* Avatar) const
{
	if (const UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(const_cast<AActor*>(Avatar)))
	{
		if (AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))))
		{
			return Target;
		}
	}
	return UGameplayStatics::GetPlayerPawn(Avatar, 0);
}

bool USkeletonMageBlinkAbility::FindDestination(const AActor* Avatar, const AActor* Target, FVector& OutLocation) const
{
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Avatar->GetWorld());
	if (!NavSystem) return false;

	const FVector TargetLocation = Target->GetActorLocation();

	for (int32 Attempt = 0; Attempt < Blink::MaxPlacementAttempts; ++Attempt)
	{
		// Alcanzable a pie desde el objetivo: el mago no aparece en una plataforma aislada donde nadie lo pueda perseguir.
		FNavLocation NavLocation;
		if (!NavSystem->GetRandomReachablePointInRadius(TargetLocation, MaxDistanceFromTarget, NavLocation)) continue;
		if (FVector::Dist2D(NavLocation.Location, TargetLocation) < MinDistanceFromTarget) continue;

		OutLocation = NavLocation.Location;
		if (const ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			OutLocation.Z += Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f;
		}
		return true;
	}

	return false;
}
