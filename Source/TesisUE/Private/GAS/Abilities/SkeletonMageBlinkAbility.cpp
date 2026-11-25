#include "GAS/Abilities/SkeletonMageBlinkAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace Blink
{
	constexpr int32 MaxPlacementAttempts = 16;
	constexpr float AimUpdateInterval = 1.f / 60.f;
	constexpr float NavProjectionHalfWidth = 50.f;
	constexpr float GroundClearance = 5.f;
}

USkeletonMageBlinkAbility::USkeletonMageBlinkAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Blink"), false)));

	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(FName("State.Dodging"), false); DodgeTag.IsValid())
	{
		ActivationOwnedTags.AddTag(DodgeTag);
	}

	AbilityInputID = EVelmaraAbilityInputID::Dodge;
	bReplacesSharedInput = true;
	PlayerCooldown = 2.f;
	PlayerEnergyCost = 5.f;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MarkerFinder(TEXT("/Game/VFX/ShamanCharge/NS_ShamanCharge.NS_ShamanCharge"));
	if (MarkerFinder.Succeeded())
	{
		AimMarkerEffect = MarkerFinder.Object;
	}
}

bool USkeletonMageBlinkAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;

	if (IsPlayerControlledAvatar(ActorInfo)) return true;

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AActor* Target = Avatar ? FindTarget(Avatar) : nullptr;
	return Target && FVector::Dist(Avatar->GetActorLocation(), Target->GetActorLocation()) <= TriggerRange;
}

void USkeletonMageBlinkAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (IsPlayerControlledAvatar(ActorInfo))
	{
		StartAiming();
		return;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	AActor* Target = Avatar ? FindTarget(Avatar) : nullptr;

	if (!Target || !FindDestination(Avatar, Target, Destination) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	BlinkTarget = Target;
	VanishThenReappear();
}

void USkeletonMageBlinkAbility::VanishThenReappear()
{
	SetVanished(true);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(VanishDuration, KINDA_SMALL_NUMBER));
	DelayTask->OnFinish.AddDynamic(this, &USkeletonMageBlinkAbility::Reappear);
	DelayTask->ReadyForActivation();
}

void USkeletonMageBlinkAbility::StartAiming()
{
	bAiming = true;
	GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTags(ActivationOwnedTags);

	GetWorld()->GetTimerManager().SetTimer(AimTimer, this, &USkeletonMageBlinkAbility::UpdateAim, Blink::AimUpdateInterval, true);

	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &USkeletonMageBlinkAbility::ReleaseAim);
	ReleaseTask->ReadyForActivation();
}

void USkeletonMageBlinkAbility::UpdateAim()
{
	if (!IsPlayerControlledAvatar(CurrentActorInfo))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	bHasAimedDestination = FindAimedDestination(Destination);

	if (!AimMarker && AimMarkerEffect)
	{
		AimMarker = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetAvatarActorFromActorInfo(), AimMarkerEffect, Destination,
			FRotator::ZeroRotator, FVector::OneVector, false);
	}
	if (!AimMarker) return;

	AimMarker->SetVisibility(bHasAimedDestination);
	AimMarker->SetWorldLocation(Destination);
	if (!AimMarker->IsActive())
	{
		AimMarker->Activate();
	}
}

void USkeletonMageBlinkAbility::ReleaseAim(float TimeHeld)
{
	if (!bAiming) return;

	UpdateAim();
	if (!bAiming) return;

	StopAiming();

	if (!bHasAimedDestination || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	VanishThenReappear();
}

void USkeletonMageBlinkAbility::StopAiming()
{
	bAiming = false;

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimTimer);
	}
	if (AimMarker)
	{
		AimMarker->DestroyComponent();
		AimMarker = nullptr;
	}
	if (UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo())
	{
		AbilitySystem->AddLooseGameplayTags(ActivationOwnedTags);
	}
}

bool USkeletonMageBlinkAbility::FindAimedDestination(FVector& OutLocation) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const APlayerController* PlayerController = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PlayerController) return false;

	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Character->GetWorld());
	if (!NavSystem) return false;

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector Origin = Character->GetActorLocation();
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * (FVector::Dist(ViewLocation, Origin) + PlayerBlinkRange);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerBlinkAim), false, Character);
	FHitResult Hit;
	const bool bHit = Character->GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);

	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector AimPoint = bHit ? Hit.ImpactPoint + Hit.ImpactNormal * CapsuleRadius : TraceEnd;

	const FVector Offset = (AimPoint - Origin).GetClampedToMaxSize2D(PlayerBlinkRange);
	const float FeetZ = Origin.Z - CapsuleHalfHeight;
	const float SearchZ = FMath::Clamp(AimPoint.Z, FeetZ - PlayerVerticalReach, FeetZ + PlayerVerticalReach);
	const FVector Extent(Blink::NavProjectionHalfWidth, Blink::NavProjectionHalfWidth, PlayerVerticalReach);

	for (int32 Step = Blink::MaxPlacementAttempts; Step > 0; --Step)
	{
		const float Fraction = static_cast<float>(Step) / Blink::MaxPlacementAttempts;
		const FVector Candidate(Origin.X + Offset.X * Fraction, Origin.Y + Offset.Y * Fraction, SearchZ);

		FNavLocation NavLocation;
		if (NavSystem->ProjectPointToNavigation(Candidate, NavLocation, Extent))
		{
			OutLocation = NavLocation.Location + FVector(0.f, 0.f, CapsuleHalfHeight + Blink::GroundClearance);
			return true;
		}
	}

	return false;
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
		else if (IsPlayerControlledAvatar(CurrentActorInfo))
		{
			Rotation.Yaw = (Destination - Avatar->GetActorLocation()).Rotation().Yaw;
		}
		Avatar->TeleportTo(Destination, Rotation);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USkeletonMageBlinkAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (bAiming)
	{
		StopAiming();
	}
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
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->StopMovementImmediately();

		if (bVanish && IsPlayerControlledAvatar(CurrentActorInfo))
		{
			Movement->DisableMovement();
			bFrozeMovement = true;
		}
		else if (!bVanish && bFrozeMovement)
		{
			Movement->SetDefaultMovementMode();
			bFrozeMovement = false;
		}
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
