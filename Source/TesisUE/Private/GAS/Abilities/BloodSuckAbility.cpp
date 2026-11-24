#include "GAS/Abilities/BloodSuckAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GAS/VelmaraAttributeSet.h"
#include "BrainComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Entities/Entity.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/OrbBase.h"
#include "Subsystems/OrbsPoolManager.h"

namespace BloodSuck
{
	bool HasHealth(const AActor* Actor)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
		const FGameplayAttribute HealthAttribute = UVelmaraAttributeSet::GetHealthAttribute();
		return ASC && (!ASC->HasAttributeSetForAttribute(HealthAttribute) || ASC->GetNumericAttribute(HealthAttribute) > 0.f);
	}
}

UBloodSuckAbility::UBloodSuckAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.BloodSuck"), false)));

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultReact(
		TEXT("/Game/Blueprints/Montages/HitReactions/AM_Katana_Blade_Damage_Front_Big_ver_D_Retargeted"));
	VictimReactMontage = DefaultReact.Object;

	static ConstructorHelpers::FClassFinder<AOrbBase> DefaultBloodOrb(TEXT("/Game/Blueprints/Items/Orbs/BP_Orb_Blood"));
	BloodOrbClass = DefaultBloodOrb.Class;
}

bool UBloodSuckAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& ActorInfo && BloodSuck::HasHealth(ActorInfo->AvatarActor.Get());
}

void UBloodSuckAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ACharacter* Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AEntity* Target = Self ? FindVictim(Cast<AEntity>(Self)) : nullptr;

	if (!Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Victim = Target;
	SetVictimFrozen(true);

	const FRotator FacingVictim(0.f, (Target->GetActorLocation() - Self->GetActorLocation()).Rotation().Yaw, 0.f);
	Self->SetActorRotation(FacingVictim);
	Target->SetActorRotation(FacingVictim);

	if (AController* Controller = Self->GetController())
	{
		Controller->SetIgnoreMoveInput(true);
		bIgnoringMoveInput = true;
	}

	if (!SuckMontage)
	{
		HandleMontageCompleted();
		return;
	}

	if (UCharacterMovementComponent* Movement = Self->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	}
	Self->GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
	Target->GetCapsuleComponent()->IgnoreActorWhenMoving(Self, true);
	Self->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
	bAttached = true;
	LungeStartLocation = Self->GetRootComponent()->GetRelativeLocation();
	LungeStartRotation = FRotator(0.f, Self->GetRootComponent()->GetRelativeRotation().Yaw, 0.f);
	LungeStartTime = GetWorld()->GetTimeSeconds();
	TickAttachment();

	if (BloodPulseCount > 0)
	{
		const float PulseInterval = FMath::Max(SuckMontage->GetPlayLength() - LungeDuration, 0.1f) / BloodPulseCount;
		GetWorld()->GetTimerManager().SetTimer(BloodPulseTimer, this, &UBloodSuckAbility::EmitBloodPulse, PulseInterval, true, LungeDuration);
	}

	if (VictimReactMontage)
	{
		const float Rate = VictimReactMontage->GetPlayLength() / FMath::Max(SuckMontage->GetPlayLength(), 0.1f);
		const float Played = Target->PlayAnimMontage(VictimReactMontage, Rate);
		UE_LOG(LogTemp, Log, TEXT("BloodSuck: victima %s reaccion %s -> %.2f s"), *GetNameSafe(Target), *GetNameSafe(VictimReactMontage), Played);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SuckMontage,
		1.f, NAME_None, true, 0.f);
	MontageTask->OnCompleted.AddDynamic(this, &UBloodSuckAbility::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UBloodSuckAbility::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UBloodSuckAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttachmentTimer);
		World->GetTimerManager().ClearTimer(BloodPulseTimer);
	}

	ACharacter* Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (bAttached && Self)
	{
		Self->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Self->SetActorRotation(FRotator(0.f, Self->GetActorRotation().Yaw, 0.f));
		if (AEntity* Target = Victim.Get())
		{
			Self->GetCapsuleComponent()->IgnoreActorWhenMoving(Target, false);
			Target->GetCapsuleComponent()->IgnoreActorWhenMoving(Self, false);
		}
		if (UCharacterMovementComponent* Movement = Self->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
	}
	bAttached = false;

	if (bIgnoringMoveInput && Self && Self->GetController())
	{
		Self->GetController()->SetIgnoreMoveInput(false);
	}
	bIgnoringMoveInput = false;

	SetVictimFrozen(false);
	Victim.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AEntity* UBloodSuckAbility::FindVictim(const AEntity* Self) const
{
	if (!Self) return nullptr;

	TArray<AActor*> Hits;
	UKismetSystemLibrary::SphereOverlapActors(Self, Self->GetActorLocation(), GrabRange,
		{UEngineTypes::ConvertToObjectType(ECC_Pawn)}, AEntity::StaticClass(), {const_cast<AEntity*>(Self)}, Hits);

	AEntity* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Hit : Hits)
	{
		AEntity* Candidate = Cast<AEntity>(Hit);
		if (!Candidate || !BloodSuck::HasHealth(Candidate) || !Self->IsHostile(Candidate)) continue;

		const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), Self->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}
	return Best;
}

void UBloodSuckAbility::SetVictimFrozen(const bool bFrozen) const
{
	AEntity* Target = Victim.Get();
	if (!Target) return;

	if (UCharacterMovementComponent* Movement = Target->GetCharacterMovement(); Movement && (bFrozen || BloodSuck::HasHealth(Target)))
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(bFrozen ? MOVE_None : MOVE_Falling);
	}

	AAIController* AIController = Cast<AAIController>(Target->GetController());
	if (!AIController) return;

	if (bFrozen)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("BloodSuck"));
		}
		return;
	}

	if (UBrainComponent* Brain = AIController->GetBrainComponent(); Brain && Target->IsAlive())
	{
		Brain->ResumeLogic(TEXT("BloodSuck"));
	}
}

void UBloodSuckAbility::TickAttachment()
{
	ACharacter* Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AEntity* Target = Victim.Get();
	USkeletalMeshComponent* SelfMesh = Self ? Self->GetMesh() : nullptr;
	USkeletalMeshComponent* TargetMesh = Target ? Target->GetMesh() : nullptr;
	if (!bAttached || !SelfMesh || !TargetMesh)
	{
		return;
	}

	const FVector BitePoint = TargetMesh->GetSocketLocation(VictimBiteBone) + Target->GetActorRotation().RotateVector(BiteOffset);
	FVector Correction = BitePoint - SelfMesh->GetSocketLocation(SelfBiteBone);

	const FVector VictimHip = TargetMesh->GetSocketLocation(VictimHipBone);
	const FVector VictimBackAxis = (TargetMesh->GetSocketLocation(VictimBiteBone) - VictimHip).GetSafeNormal();
	const FVector VictimBehind = -Target->GetActorForwardVector();
	const FVector OutOfVictimBack = (VictimBehind - VictimBackAxis * FVector::DotProduct(VictimBehind, VictimBackAxis)).GetSafeNormal();
	float Penetration = 0.f;
	for (const FName& BodyBone : SelfBodyBones)
	{
		const FVector BodyPoint = SelfMesh->GetSocketLocation(BodyBone) + Correction;
		Penetration = FMath::Max(Penetration, BodyClearance - FVector::DotProduct(BodyPoint - VictimHip, OutOfVictimBack));
	}
	Correction += OutOfVictimBack * Penetration;

	const FVector AlignedWorldLocation = Self->GetActorLocation() + Correction;
	const FVector AlignedRelativeLocation = Target->GetActorTransform().InverseTransformPositionNoScale(AlignedWorldLocation);

	const float LungeElapsed = FMath::Min(GetWorld()->GetTimeSeconds() - LungeStartTime, LungeDuration);
	const float Alpha = LungeDuration > 0.f ? FMath::InterpEaseOut(0.f, 1.f, LungeElapsed / LungeDuration, 2.f) : 1.f;
	Self->SetActorRelativeLocation(FMath::Lerp(LungeStartLocation, AlignedRelativeLocation, Alpha));
	Self->SetActorRelativeRotation(FQuat::Slerp(LungeStartRotation.Quaternion(), FQuat::Identity, Alpha));

	AttachmentTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UBloodSuckAbility::TickAttachment);
}

void UBloodSuckAbility::EmitBloodPulse() const
{
	AEntity* Target = Victim.Get();
	AActor* Self = GetAvatarActorFromActorInfo();
	if (!Target || !Self || !Target->GetMesh()) return;

	const FVector BitePoint = Target->GetMesh()->GetSocketLocation(VictimBiteBone);

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target); TargetASC && BleedCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = BitePoint;
		CueParameters.Normal = (FVector::UpVector - Target->GetActorForwardVector() * 0.5f).GetSafeNormal();
		CueParameters.Instigator = Self;
		CueParameters.EffectCauser = Self;
		TargetASC->ExecuteGameplayCue(BleedCueTag, CueParameters);
	}

	UOrbsPoolManager* OrbPool = GetWorld()->GetSubsystem<UOrbsPoolManager>();
	if (!OrbPool || !BloodOrbClass) return;

	if (AOrbBase* Orb = Cast<AOrbBase>(OrbPool->SpawnOrbFromPool(BloodOrbClass, BitePoint, FRotator::ZeroRotator, Target)))
	{
		Orb->LaunchToTarget(Self);
	}
}

void UBloodSuckAbility::HandleMontageCompleted()
{
	if (!IsActive()) return;

	if (HealEffect)
	{
		BP_ApplyGameplayEffectToOwner(HealEffect);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBloodSuckAbility::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
