#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterStates.h"
#include "PlayerMainAnimInstance.generated.h"

class APlayerMain;
class UCharacterMovementComponent;
class UPlayerFormComponent;

UCLASS()
class TESISUE_API UPlayerMainAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	APlayerMain* PlayerMain;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	UCharacterMovementComponent* PlayerMainCharacterMovement;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerForm")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool IsFalling;

	UPROPERTY(BlueprintReadOnly, Category = "Movement | Character State")
	ECharacterStates CharacterState;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerForm")
	EPlayerForm PlayerForm;
};
