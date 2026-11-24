#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Items/Weapons/DualSword.h"
#include "AnimNotifyState_DualSwordHand.generated.h"

UCLASS(meta = (DisplayName = "Dual Sword Hand"))
class TESISUE_API UAnimNotifyState_DualSwordHand : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dual Sword")
	EDualSwordHand Hand = EDualSwordHand::Both;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	static void SetHand(const USkeletalMeshComponent* MeshComp, EDualSwordHand NewHand);
};
