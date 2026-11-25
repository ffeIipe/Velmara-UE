#include "Features/GlobalEffectsSystem/Animation/AnimNotifyState_ChargeZoom.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TargetingComponent.h"
#include "Entities/Entity.h"
#include "Features/GlobalEffectsSystem/Core/EffectsManager.h"

namespace
{
	UEffectsManager* FindEffectsManager(const USkeletalMeshComponent* MeshComp)
	{
		const UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UEffectsManager>() : nullptr;
	}

	float GetEffectivePlayRate(const USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation)
	{
		const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
		const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
		if (!Montage || !AnimInstance) return 1.0f;

		const float PlayRate = FMath::Abs(AnimInstance->Montage_GetPlayRate(Montage) * Montage->RateScale);
		return PlayRate > KINDA_SMALL_NUMBER ? PlayRate : 1.0f;
	}
}

UAnimNotifyState_ChargeZoom::UAnimNotifyState_ChargeZoom()
{
	ZoomTag = FGameplayTag::RequestGameplayTag(TEXT("Effect.Camera.Zoom.Charge"), false);
}

void UAnimNotifyState_ChargeZoom::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const AEntity* Entity = MeshComp ? Cast<AEntity>(MeshComp->GetOwner()) : nullptr;
	if (!Entity || !Entity->IsPlayerControlled()) return;

	const UTargetingComponent* Targeting = Entity->GetTargetingComponent();
	if (!Targeting || (!Targeting->GetCurrentTarget() && !Targeting->FindSoftLockTarget(TargetSearchRadius))) return;

	if (UEffectsManager* Effects = FindEffectsManager(MeshComp))
	{
		Effects->BeginHeldCameraZoom(ZoomTag, TotalDuration / GetEffectivePlayRate(MeshComp, Animation), MeshComp);
	}
}

void UAnimNotifyState_ChargeZoom::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UEffectsManager* Effects = FindEffectsManager(MeshComp))
	{
		Effects->EndHeldCameraZoom(MeshComp);
	}
}

FString UAnimNotifyState_ChargeZoom::GetNotifyName_Implementation() const
{
	return TEXT("Charge Camera Zoom");
}
