// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "UObject/Object.h"
#include "CombatStrategy.generated.h"

class UCombatStrategyData;
class UCommand;


UCLASS(Blueprintable)
class TESISUE_API UCombatStrategy : public UObject
{
	GENERATED_BODY()
	
public:
	void InitializeStrategy();
	
	void Strategy_UseFirstCommand(AActor* User);
	void Strategy_UseSecondCommand(AActor* User);
	void Strategy_UseAbility(AActor* User);
	void Strategy_Dodge(AActor* User);

	void SetCurrentValues(const TObjectPtr<AEntity>& Entity) const;
	
	UFUNCTION()
	void ResetCommands();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCombatStrategyData* CombatStrategyData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UDataTable> CommandsDataTable;

private:
	UPROPERTY()
	UCommand* FirstCommandInstance;

	UPROPERTY()
	UCommand* SecondCommandInstance;

	UPROPERTY()
	UCommand* AbilityCommandInstance;

	UPROPERTY()
	UCommand* DodgeCommandInstance;
	
	TSubclassOf<UCommand> FirstCommandClass;
	TSubclassOf<UCommand> SecondCommandClass;
	TSubclassOf<UCommand> AbilityCommandClass;
	TSubclassOf<UCommand> DodgeCommandClass;

	bool bWasInitialized = false;
};