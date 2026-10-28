#include "Features/BossSystem/Animation/AnimNotifyState_BossHitWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"

namespace
{
	UBossCombatComponent* FindBossCombatFromMesh(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UBossCombatComponent>() : nullptr;
	}
}

void UAnimNotifyState_BossHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UBossCombatComponent* Combat = FindBossCombatFromMesh(MeshComp))
	{
		Combat->BeginNotifyWindow(this, Window);
	}
}

void UAnimNotifyState_BossHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (UBossCombatComponent* Combat = FindBossCombatFromMesh(MeshComp))
	{
		Combat->EndNotifyWindow(this);
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

FString UAnimNotifyState_BossHitWindow::GetNotifyName_Implementation() const
{
	switch (Window.Shape)
	{
	case EBossHitShape::Sphere:		return TEXT("Boss Hit (Sphere)");
	case EBossHitShape::Projectile:	return TEXT("Boss Projectile");
	default:						return TEXT("Boss Hit (Weapon)");
	}
}
