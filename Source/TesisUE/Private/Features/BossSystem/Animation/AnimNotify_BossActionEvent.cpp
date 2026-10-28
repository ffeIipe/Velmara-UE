#include "Features/BossSystem/Animation/AnimNotify_BossActionEvent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"

void UAnimNotify_BossActionEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UBossCombatComponent* Combat = Owner ? Owner->FindComponentByClass<UBossCombatComponent>() : nullptr)
	{
		Combat->TriggerActionEventFromNotify();
	}
}
