#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputData.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FInputs
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Movement")
	UInputAction* DodgeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InventoryAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* ToggleWeaponAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Skill1;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Skill2;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Skill3;

	UPROPERTY(EditDefaultsOnly, Category = "Input | Actions")
	UInputAction* InputAction_Skill4;
};

UCLASS()
class TESISUE_API UInputData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	FInputs Inputs;
};