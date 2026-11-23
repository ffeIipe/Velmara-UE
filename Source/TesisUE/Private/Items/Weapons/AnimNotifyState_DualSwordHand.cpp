#include "Items/Weapons/AnimNotifyState_DualSwordHand.h"

#include "Entities/Entity.h"

void UAnimNotifyState_DualSwordHand::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	SetHand(MeshComp, Hand);
}

void UAnimNotifyState_DualSwordHand::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	SetHand(MeshComp, EDualSwordHand::Both);
}

FString UAnimNotifyState_DualSwordHand::GetNotifyName_Implementation() const
{
	return UEnum::GetDisplayValueAsText(Hand).ToString();
}

void UAnimNotifyState_DualSwordHand::SetHand(const USkeletalMeshComponent* MeshComp, const EDualSwordHand NewHand)
{
	if (!MeshComp) return;

	const AEntity* Entity = Cast<AEntity>(MeshComp->GetOwner());
	if (!Entity) return;

	if (ADualSword* Sword = Cast<ADualSword>(Entity->GetCurrentItem().GetObject()))
	{
		Sword->SetActiveHand(NewHand);
	}
}
