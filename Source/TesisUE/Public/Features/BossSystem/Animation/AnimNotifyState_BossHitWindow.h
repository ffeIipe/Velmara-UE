#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Features/BossSystem/Data/BossTypes.h"
#include "AnimNotifyState_BossHitWindow.generated.h"

/*
 * Ventana de golpe precisa para los montajes del boss. Si un montaje tiene al menos una,
 * se ignoran las ventanas por tiempo de la tabla de ataques (StartTime/EndTime no se usan acá).
 */
UCLASS(meta = (DisplayName = "Boss Hit Window"))
class TESISUE_API UAnimNotifyState_BossHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	FBossHitWindow Window;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
