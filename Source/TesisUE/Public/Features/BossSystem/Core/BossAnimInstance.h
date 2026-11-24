#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BossAnimInstance.generated.h"

UCLASS()
class TESISUE_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	int32 Stage = 0;
};
