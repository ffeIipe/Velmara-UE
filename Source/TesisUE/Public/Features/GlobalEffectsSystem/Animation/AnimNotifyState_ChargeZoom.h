#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_ChargeZoom.generated.h"

UCLASS(meta = (DisplayName = "Charge Camera Zoom"))
class TESISUE_API UAnimNotifyState_ChargeZoom : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_ChargeZoom();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge Zoom", meta = (Categories = "Effect.Camera.Zoom"))
	FGameplayTag ZoomTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge Zoom", meta = (ClampMin = "0.0"))
	float TargetSearchRadius = 600.0f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
