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

	const FVector ToVictim = Target->GetActorLocation() - Self->GetActorLocation();
	Self->SetActorRotation(FRotator(0.f, ToVictim.Rotation().Yaw, 0.f));

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
	Self->AttachToActor(Target, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Self->SetActorRelativeLocation(FVector(-BackDistance, 0.f,
		Self->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	Self->SetActorRelativeRotation(FRotator::ZeroRotator);
	bAttached = true;

	if (USkeletalMeshComponent* SelfMesh = Self->GetMesh(); SelfMesh && Target->GetMesh())
	{
		SavedMeshTransform = SelfMesh->GetRelativeTransform();
		SelfMesh->AttachToComponent(Target->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, AttachBone);
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
	ACharacter* Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (bAttached && Self)
	{
		if (USkeletalMeshComponent* SelfMesh = Self->GetMesh())
		{
			SelfMesh->AttachToComponent(Self->GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			SelfMesh->SetRelativeTransform(SavedMeshTransform);
		}

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
	AAIController* AIController = Target ? Cast<AAIController>(Target->GetController()) : nullptr;
	if (!AIController) return;

	if (bFrozen)
	{
		AIController->StopMovement();
		AIController->SetFocus(GetAvatarActorFromActorInfo(), EAIFocusPriority::Gameplay);
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("BloodSuck"));
		}
		return;
	}

	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	if (UBrainComponent* Brain = AIController->GetBrainComponent(); Brain && Target->IsAlive())
	{
		Brain->ResumeLogic(TEXT("BloodSuck"));
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
