#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BossAnimInstance.generated.h"

/*
 * Base opcional para el AnimBP del boss. BossAnimationBluepint del pack castea a BossCharacter (el boss de demo),
 * así que con ABossEnemy su Speed queda en 0: reparentalo a esta clase y usá GroundSpeed en el Blendspace.
 */
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
