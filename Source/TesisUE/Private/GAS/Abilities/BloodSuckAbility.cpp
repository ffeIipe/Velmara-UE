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
	// IsAlive() mira State.Dead, que nunca se aplica (GA_Death no está otorgada): la vida es la fuente confiable.
	bool HasHealth(const AActor* Actor)
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor));
		const FGameplayAttribute HealthAttribute = UVelmaraAttributeSet::GetHealthAttribute();
		// Hay enemigos colocados en el nivel sin AttributeSet registrado: sin atributo no se los da por muertos.
		return ASC && (!ASC->HasAttributeSetForAttribute(HealthAttribute) || ASC->GetNumericAttribute(HealthAttribute) > 0.f);
	}
}

UBloodSuckAbility::UBloodSuckAbility()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.BloodSuck"), false)));

	// Valor por defecto; se puede cambiar en GA_BloodSuck.
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

	// Velmara se teletransporta a la espalda del enemigo y queda pegada a ella mientras dura el montage (la cámara la sigue
	// porque va con el enemigo). El root motion del montage se anula (escala 0 en la task) para que no la separe.
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

	// El mesh va al hueso del enemigo para que siga su animación; la cápsula (y la cámara) quedan pegadas al actor.
	if (USkeletalMeshComponent* SelfMesh = Self->GetMesh(); SelfMesh && Target->GetMesh())
	{
		SavedMeshTransform = SelfMesh->GetRelativeTransform();
		// KeepWorld: el mesh ya está en la pose "a la espalda" (por la cápsula); así el hueso solo aporta el movimiento, sin offsets a ojo.
		SelfMesh->AttachToComponent(Target->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, AttachBone);
	}

	// El enemigo ejecuta su animación estirada para que dure lo mismo que el chupar.
	if (VictimReactMontage)
	{
		const float Rate = VictimReactMontage->GetPlayLength() / FMath::Max(SuckMontage->GetPlayLength(), 0.1f);
		const float Played = Target->PlayAnimMontage(VictimReactMontage, Rate);
		UE_LOG(LogTemp, Log, TEXT("BloodSuck: victima %s reaccion %s -> %.2f s"), *GetNameSafe(Target), *GetNameSafe(VictimReactMontage), Played);
	}

	// Si la interrumpen (hit react, muerte) no cura: el cooldown se gasta igual.
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
	// Pase lo que pase (interrumpida, muerte), Velmara se despega, recupera el control y el enemigo se descongela.
	ACharacter* Self = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (bAttached && Self)
	{
		// El mesh vuelve a la cápsula con su transform original antes de soltar el actor.
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
