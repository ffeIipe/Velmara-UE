#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Enemy/Enemy.h"
#include "EnemyCharacterAnimInstance.generated.h"

class UCharacterMovementComponent;

UCLASS()
class TESISUE_API UEnemyCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds);


protected:
	UPROPERTY()
	AEnemy* OwningEnemy; 

	UPROPERTY()
	UCharacterMovementComponent* OwningMovementComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bIsFalling;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float GroundSpeed;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bHasAcceleration;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkRight;

	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	float WalkForward;

	UFUNCTION(BlueprintCallable)
	float const CalculateWalkRight();

	UFUNCTION(BlueprintCallable)
	float const CalculateWalkForward();

	UPROPERTY(BlueprintReadWrite)
	EEnemyState CurrentEnemyState;
};
