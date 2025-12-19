#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "ResetPlayer.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API AResetPlayer : public ATrigger
{
	GENERATED_BODY()

	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult) override;
};
