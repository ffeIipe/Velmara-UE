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

namespace BloodSuck
{
	// IsAlive() mira State.Dead, que nunca se aplica (GA_Death no está otorgada): la vida es la fuente confiable.
	bool HasHealth(const AActor* Actor)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
		return ASC && ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) > 0.f;
	}
}

UBloodSuckAbility::UBloodSuckAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.BloodSuck"), false)));
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

	// Sin a quién morder no se gasta el cooldown.
	if (!Target || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Victim = Target;
	SetVictimFrozen(true);

	const FVector ToVictim = Target->GetActorLocation() - Self->GetActorLocation();
	Self->SetActorRotation(FRotator(0.f, ToVictim.Rotation().Yaw, 0.f));

	// La estocada del montage avanza MontageLungeDistance y vuelve: la escalamos para que termine pegada al enemigo.
	const float LungeScale = FMath::Clamp((ToVictim.Size2D() - BiteDistance) / MontageLungeDistance, 0.f, 3.f);

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

	// Si la interrumpen (hit react, muerte) no cura: el cooldown se gasta igual.
	// La escala va por la task: al arrancar pisa la del Character y al terminar la vuelve a 1.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SuckMontage,
		1.f, NAME_None, true, LungeScale);
	MontageTask->OnCompleted.AddDynamic(this, &UBloodSuckAbility::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UBloodSuckAbility::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UBloodSuckAbility::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UBloodSuckAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	// Pase lo que pase (interrumpida, muerte), Velmara recupera el control y el enemigo se descongela.
	const APawn* Self = Cast<APawn>(GetAvatarActorFromActorInfo());
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
	// Si murió mientras lo chupaban no se le reanuda el árbol.
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
