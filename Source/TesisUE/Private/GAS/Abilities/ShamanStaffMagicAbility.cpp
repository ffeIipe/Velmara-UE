#include "GAS/Abilities/ShamanStaffMagicAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/TargetingComponent.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Actors/BossProjectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/CustomPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace ShamanStaffMagic
{
	constexpr float PlayerProjectileCollisionRadius = 24.f;
	constexpr float AIProjectileCollisionRadius = 24.f;
	constexpr float SpawnCheckOriginHeightOffset = 40.f;
	constexpr float WallPullbackOffset = 2.f;
	constexpr float MinPlayerRecoveryAfterFire = 0.1f;

	bool TrySetCharacterWantsToStrafe(AActor* Avatar, const bool bNewValue, bool& OutPreviousValue)
	{
		if (!Avatar) return false;

		FStructProperty* InputStateProp = FindFProperty<FStructProperty>(Avatar->GetClass(), TEXT("CharacterInputState"));
		if (!InputStateProp || !InputStateProp->Struct) return false;

		void* StructMemory = InputStateProp->ContainerPtrToValuePtr<void>(Avatar);
		if (!StructMemory) return false;

		for (TFieldIterator<FBoolProperty> It(InputStateProp->Struct); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("WantsToStrafe")))
			{
				OutPreviousValue = It->GetPropertyValue_InContainer(StructMemory);
				It->SetPropertyValue_InContainer(StructMemory, bNewValue);
				return true;
			}
		}

		return false;
	}
}

UShamanStaffMagicAbility::UShamanStaffMagicAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ProjectileClass = ABossProjectile::StaticClass();
	PlayerCooldown = 1.f;
	PlayerEnergyCost = 5.f;

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
	LockingTag = FGameplayTag::RequestGameplayTag(FName("State.Locking"), false);
}

void UShamanStaffMagicAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bActivatedByPlayer = IsPlayerControlledAvatar(ActorInfo);
	if (bActivatedByPlayer)
	{
		StartPlayerCastRotation();
	}

	const float Rate = GetAttackSpeedMultiplier();
	const float BaseFireDelay = bActivatedByPlayer ? FireDelay : FMath::Max(AIFireDelay, FireDelay);
	const float WindupDuration = BaseFireDelay / Rate;
	const float WindupMontageRate = WindupDuration > KINDA_SMALL_NUMBER ? FireDelay / WindupDuration : Rate;
	CastMontageRate = Rate;

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, WindupDuration);
	DelayTask->OnFinish.AddDynamic(this, &UShamanStaffMagicAbility::FireProjectile);
	DelayTask->ReadyForActivation();

	if (!bActivatedByPlayer)
	{
		StartTelegraph(WindupDuration);
	}
	else
	{
		UAbilityTask_WaitDelay* RecoveryTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(PlayerCooldown, WindupDuration + ShamanStaffMagic::MinPlayerRecoveryAfterFire));
		RecoveryTask->OnFinish.AddDynamic(this, &UShamanStaffMagicAbility::HandlePlayerCastRecovered);
		RecoveryTask->ReadyForActivation();
	}

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage, WindupMontageRate);
		MontageTask->OnCompleted.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->OnInterrupted.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->OnCancelled.AddDynamic(this, &UShamanStaffMagicAbility::HandleMontageEnded);
		MontageTask->ReadyForActivation();
	}
}

void UShamanStaffMagicAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	StopPlayerCastRotation();
	StopTelegraph();
	bActivatedByPlayer = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShamanStaffMagicAbility::FireProjectile()
{
	if (!IsActive()) return;

	StopTelegraph();
	RestoreCastMontageRate();

	AActor* Avatar = GetAvatarActorFromActorInfo();
	const bool bPlayerCast = bActivatedByPlayer;

	if (bPlayerCast && !IsPlayerControlledAvatar(CurrentActorInfo))
	{
		StopPlayerCastRotation();
		if (!CastMontage)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	UActorPoolSubsystem* Pool = Avatar ? Avatar->GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr;
	if (Pool && ProjectileClass)
	{
		const float ProbeRadius = bPlayerCast ? ShamanStaffMagic::PlayerProjectileCollisionRadius : ShamanStaffMagic::AIProjectileCollisionRadius;
		const FVector SpawnLocation = GetSafeSpawnLocation(Avatar, ProbeRadius);

		FVector Direction = Avatar->GetActorForwardVector();
		AActor* HomingTarget = nullptr;
		float Speed = ProjectileSpeed;
		FBossDamageParams EffectiveDamageParams = DamageParams;

		if (bPlayerCast)
		{
			AActor* LockedTarget = nullptr;
			const FVector AimPoint = ResolvePlayerAimPoint(Avatar, LockedTarget);
			Direction = (AimPoint - SpawnLocation).GetSafeNormal();

			if (!LockedTarget)
			{
				const APawn* Pawn = Cast<APawn>(Avatar);
				if (const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
				{
					FVector CameraLocation = FVector::ZeroVector;
					FRotator CameraRotation = FRotator::ZeroRotator;
					PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
					const FVector CameraForward = CameraRotation.Vector();
					if (Direction.IsNearlyZero() || FVector::DotProduct(AimPoint - SpawnLocation, CameraForward) <= 0.f)
					{
						Direction = CameraForward;
					}
				}
			}

			if (Direction.IsNearlyZero())
			{
				Direction = Avatar->GetActorForwardVector();
			}

			if (LockedTarget && bPlayerHomingOnLock && PlayerHomingAcceleration > 0.f)
			{
				HomingTarget = LockedTarget;
			}

			Speed = PlayerProjectileSpeed;
			EffectiveDamageParams.Damage = PlayerDamage;
			EffectiveDamageParams.bOnlyPlayerControlled = false;
		}
		else if (const AActor* Target = FindAITarget(Avatar))
		{
			Direction = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
		}

		if (ABossProjectile* Projectile = Cast<ABossProjectile>(Pool->GetActorFromPool(ProjectileClass, FTransform(Direction.Rotation(), SpawnLocation))))
		{
			Projectile->SetPierceTargets(false);
			if (bPlayerCast)
			{
				Projectile->SetCollisionRadius(ShamanStaffMagic::PlayerProjectileCollisionRadius);
			}
			Projectile->Launch(Avatar, Direction, Speed, ProjectileLifetime, EffectiveDamageParams);
			if (HomingTarget)
			{
				Projectile->SetHomingTarget(HomingTarget, PlayerHomingAcceleration);
			}
		}
	}

	if (!CastMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShamanStaffMagicAbility::HandleMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShamanStaffMagicAbility::HandlePlayerCastRecovered()
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	const bool bRepeatCast = Spec && Spec->InputPressed && IsPlayerControlledAvatar(CurrentActorInfo);
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpecHandle SpecHandle = CurrentSpecHandle;

	NextPlayerCastTime = 0.0;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

	if (bRepeatCast && AbilitySystem)
	{
		AbilitySystem->TryActivateAbility(SpecHandle);
	}
}

void UShamanStaffMagicAbility::StartTelegraph(const float ChargeDuration)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	USceneComponent* StaffTip = FindStaffTipComponent(Avatar);
	USceneComponent* AttachParent = StaffTip ? StaffTip : Avatar->GetRootComponent();
	const FName AttachSocket = StaffTip ? StaffTipSocket : NAME_None;
	if (!AttachParent) return;

	if (APawn* AvatarPawn = Cast<APawn>(Avatar))
	{
		AvatarPawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &UShamanStaffMagicAbility::HandleAvatarControllerChanged);
	}

	if (ChargeEffect)
	{
		ChargeComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(ChargeEffect, AttachParent, AttachSocket, FVector::ZeroVector,
			FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true, false);
		if (ChargeComponent)
		{
			ChargeComponent->SetVariableFloat(ChargeDurationParameter, ChargeDuration);
			ChargeComponent->Activate(true);
		}
	}

	if (ChargeSound)
	{
		ChargeAudio = UGameplayStatics::SpawnSoundAttached(ChargeSound, AttachParent, AttachSocket);
	}

	if (TellEffect)
	{
		UAbilityTask_WaitDelay* TellTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(ChargeDuration - TellLeadTime, 0.f));
		TellTask->OnFinish.AddDynamic(this, &UShamanStaffMagicAbility::PlayTell);
		TellTask->ReadyForActivation();
	}
}

void UShamanStaffMagicAbility::PlayTell()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsActive() || !Avatar || !TellEffect || IsPlayerControlledAvatar(CurrentActorInfo)) return;

	if (USceneComponent* StaffTip = FindStaffTipComponent(Avatar))
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(TellEffect, StaffTip, StaffTipSocket, FVector::ZeroVector,
			FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(Avatar, TellEffect, GetStaffTipLocation(Avatar));
}

void UShamanStaffMagicAbility::HandleAvatarControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (NewController && NewController->IsPlayerController())
	{
		StopTelegraph();
	}
}

void UShamanStaffMagicAbility::StopTelegraph()
{
	if (APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		AvatarPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UShamanStaffMagicAbility::HandleAvatarControllerChanged);
	}

	if (ChargeComponent)
	{
		ChargeComponent->DestroyComponent();
		ChargeComponent = nullptr;
	}

	if (ChargeAudio)
	{
		ChargeAudio->Stop();
		ChargeAudio = nullptr;
	}
}

void UShamanStaffMagicAbility::RestoreCastMontageRate() const
{
	UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	if (CastMontage && AnimInstance && AnimInstance->Montage_IsPlaying(CastMontage))
	{
		AnimInstance->Montage_SetPlayRate(CastMontage, CastMontageRate);
	}
}

void UShamanStaffMagicAbility::StartPlayerCastRotation()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || IsLockingTarget(Avatar)) return;

	bool bPreviousWantsToStrafe = false;
	if (ShamanStaffMagic::TrySetCharacterWantsToStrafe(Avatar, true, bPreviousWantsToStrafe))
	{
		if (!bPreviousWantsToStrafe)
		{
			bSavedWantsToStrafe = false;
			bOverrodeWantsToStrafe = true;
		}
	}

	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
			bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
			Movement->bOrientRotationToMovement = false;
			Movement->bUseControllerDesiredRotation = true;
			bOverrodeMovementRotation = true;
		}
	}
}

void UShamanStaffMagicAbility::StopPlayerCastRotation()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();

	if (bOverrodeWantsToStrafe)
	{
		bool bUnused = false;
		ShamanStaffMagic::TrySetCharacterWantsToStrafe(Avatar, bSavedWantsToStrafe, bUnused);
		bOverrodeWantsToStrafe = false;
	}

	if (bOverrodeMovementRotation)
	{
		if (Avatar && !IsLockingTarget(Avatar))
		{
			if (ACharacter* Character = Cast<ACharacter>(Avatar))
			{
				if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
				{
					Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
					Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
				}
			}
		}
		bOverrodeMovementRotation = false;
	}
}

bool UShamanStaffMagicAbility::IsLockingTarget(const AActor* Avatar) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && LockingTag.IsValid() && ASC->HasMatchingGameplayTag(LockingTag);
}

AActor* UShamanStaffMagicAbility::FindLockedTarget(const AActor* Avatar) const
{
	if (!IsLockingTarget(Avatar))
	{
		return nullptr;
	}

	const AEntity* Entity = Cast<AEntity>(Avatar);
	const UTargetingComponent* Targeting = Entity ? Entity->GetTargetingComponent() : nullptr;
	AActor* Target = Targeting ? Targeting->GetCurrentTarget() : nullptr;
	if (!IsValid(Target))
	{
		return nullptr;
	}

	if (const AEntity* TargetEntity = Cast<AEntity>(Target); TargetEntity && !TargetEntity->IsAlive())
	{
		return nullptr;
	}

	return Target;
}

AActor* UShamanStaffMagicAbility::FindAITarget(const AActor* Avatar) const
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

FVector UShamanStaffMagicAbility::ResolvePlayerAimPoint(const AActor* Avatar, AActor*& OutLockedTarget) const
{
	if (AActor* LockedTarget = FindLockedTarget(Avatar))
	{
		OutLockedTarget = LockedTarget;
		return LockedTarget->GetActorLocation();
	}

	OutLockedTarget = nullptr;
	const APawn* Pawn = Cast<APawn>(Avatar);
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * PlayerAimTraceDistance;
	}

	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector();

	const float DistanceToAvatar = FMath::Max(0.f, FVector::DotProduct(Avatar->GetActorLocation() - CameraLocation, CameraForward));
	const FVector TraceStart = CameraLocation + CameraForward * DistanceToAvatar;
	const FVector TraceEnd = CameraLocation + CameraForward * PlayerAimTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShamanPlayerAim), false, Avatar);
	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors, true, true);
	QueryParams.AddIgnoredActors(AttachedActors);
	if (const ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(PlayerController))
	{
		if (CustomPC->IsPossessing() && CustomPC->GetOriginalBody())
		{
			QueryParams.AddIgnoredActor(CustomPC->GetOriginalBody());
		}
	}

	FVector ResolvedPoint = TraceEnd;
	FHitResult WorldHit;
	if (Avatar->GetWorld()->LineTraceSingleByChannel(WorldHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && WorldHit.bBlockingHit)
	{
		ResolvedPoint = WorldHit.ImpactPoint;
	}

	TArray<FHitResult> PawnHits;
	const FCollisionObjectQueryParams PawnQuery(ECC_Pawn);
	if (Avatar->GetWorld()->LineTraceMultiByObjectType(PawnHits, TraceStart, ResolvedPoint, PawnQuery, QueryParams))
	{
		const AEntity* AvatarEntity = Cast<AEntity>(Avatar);
		for (const FHitResult& PawnHit : PawnHits)
		{
			AActor* HitActor = PawnHit.GetActor();
			if (!IsValid(HitActor) || HitActor->IsHidden())
			{
				continue;
			}

			const AEntity* TargetEntity = Cast<AEntity>(HitActor);
			if (TargetEntity && (!TargetEntity->IsAlive() || (AvatarEntity && !AvatarEntity->IsHostile(TargetEntity))))
			{
				continue;
			}

			ResolvedPoint = PawnHit.ImpactPoint;
			break;
		}
	}

	return ResolvedPoint;
}

USceneComponent* UShamanStaffMagicAbility::FindStaffTipComponent(const AActor* Avatar) const
{
	TArray<AActor*> ActorsToSearch;
	Avatar->GetAttachedActors(ActorsToSearch, true, true);
	ActorsToSearch.Insert(const_cast<AActor*>(Avatar), 0);

	for (const AActor* Actor : ActorsToSearch)
	{
		TInlineComponentArray<USceneComponent*> Components(Actor);
		for (USceneComponent* Component : Components)
		{
			if (Component->DoesSocketExist(StaffTipSocket))
			{
				return Component;
			}
		}
	}
	return nullptr;
}

FVector UShamanStaffMagicAbility::GetStaffTipLocation(const AActor* Avatar) const
{
	if (const USceneComponent* StaffTip = FindStaffTipComponent(Avatar))
	{
		return StaffTip->GetSocketLocation(StaffTipSocket);
	}
	return Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.f;
}

FVector UShamanStaffMagicAbility::GetSafeSpawnLocation(const AActor* Avatar, const float ProbeRadius) const
{
	const FVector RawLocation = GetStaffTipLocation(Avatar);
	const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	if (!World)
	{
		return RawLocation;
	}

	const FVector SafeOrigin = Avatar->GetActorLocation() + FVector(0.f, 0.f, ShamanStaffMagic::SpawnCheckOriginHeightOffset);
	if (FVector::DistSquared(SafeOrigin, RawLocation) <= KINDA_SMALL_NUMBER)
	{
		return RawLocation;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShamanStaffSpawnCheck), false, Avatar);
	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors, true, true);
	QueryParams.AddIgnoredActors(AttachedActors);
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const ACustomPlayerController* CustomPC = Cast<ACustomPlayerController>(Pawn->GetController()))
		{
			if (CustomPC->IsPossessing() && CustomPC->GetOriginalBody())
			{
				QueryParams.AddIgnoredActor(CustomPC->GetOriginalBody());
			}
		}
	}

	FCollisionObjectQueryParams BlockingObjects;
	BlockingObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	BlockingObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	BlockingObjects.AddObjectTypesToQuery(ECC_PhysicsBody);
	BlockingObjects.AddObjectTypesToQuery(ECC_Destructible);
	BlockingObjects.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	TArray<FHitResult> Hits;
	const float EffectiveRadius = FMath::Max(ProbeRadius, 1.f);
	if (World->SweepMultiByObjectType(Hits, SafeOrigin, RawLocation, FQuat::Identity, BlockingObjects, FCollisionShape::MakeSphere(EffectiveRadius), QueryParams))
	{
		for (const FHitResult& Hit : Hits)
		{
			const UPrimitiveComponent* HitComp = Hit.GetComponent();
			if (!HitComp || HitComp->GetCollisionResponseToChannel(ECC_WorldDynamic) != ECR_Block)
			{
				continue;
			}
			if (Hit.GetActor() && Hit.GetActor()->IsA<ABossProjectile>())
			{
				continue;
			}
			if (Hit.bStartPenetrating)
			{
				return SafeOrigin;
			}
			return Hit.Location + Hit.Normal * ShamanStaffMagic::WallPullbackOffset;
		}
	}

	return RawLocation;
}
