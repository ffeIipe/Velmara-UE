#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "UObject/Object.h"
#include "CombatStrategy.generated.h"

class UCombatStrategyData;
class UCommand;

UENUM(Blueprintable)
enum EInputCommandType: uint8
{
	ECT_None			UMETA(DisplayName = "None"),
	ECT_PrimaryAttack	UMETA(DisplayName = "PrimaryAttack"),
	ECT_SecondaryAttack UMETA(DisplayName = "SecondaryAttack"),
	ECT_LaunchAttack	UMETA(DisplayName = "LaunchAttack"),
	ECT_Ability			UMETA(DisplayName = "Ability"),
	ECT_Dodge			UMETA(DisplayName = "Dodge")
};

UCLASS(Blueprintable)
class TESISUE_API UCombatStrategy : public UObject
{
	GENERATED_BODY()
	
public:
	void InitializeStrategy();

	void Strategy_UseCommand(AEntity* User, const EInputCommandType& CommandType);
	
	void SetCurrentValues(const TObjectPtr<AEntity>& Entity) const;

	UFUNCTION()
	void ResetCommands();
	
	UPROPERTY()
	TMap<TEnumAsByte<EInputCommandType>, TObjectPtr<UCommand>> CommandsInstances;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCombatStrategyData* CombatStrategyData;


private:
	bool bWasInitialized = false;
};