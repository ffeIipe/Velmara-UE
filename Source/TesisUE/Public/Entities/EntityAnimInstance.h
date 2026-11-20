#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EntityAnimInstance.generated.h"

class UAbilitySystemComponent;
class UCharacterMovementComponent;
class AEntity;

UCLASS()
class TESISUE_API UEntityAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	AEntity* EntityOwner;

	UPROPERTY(BlueprintReadOnly)
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly)
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsLocking = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsDodging = false;
	
};
