#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CharacterWeaponStates.h"
#include "CharacterStateComponent.generated.h"

USTRUCT(BlueprintType)
struct TESISUE_API FCharacterStates
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character State")
	ECharacterWeaponStates WeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Action")
	ECharacterActionsStates Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Form")
	ECharacterModeStates Mode;
	
	FCharacterStates()
		: WeaponState(ECharacterWeaponStates::ECWS_Unequipped)
		, Action(ECharacterActionsStates::ECAS_Nothing)
		, Mode(ECharacterModeStates::ECMS_Human)
	{
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UCharacterStateComponent : public UActorComponent
{
	GENERATED_BODY()	

public:
	UCharacterStateComponent();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character States")
	FCharacterStates CurrentStates;

	UFUNCTION(BlueprintCallable, Category = "Character States | Character State")
	ECharacterWeaponStates SetHumanState(ECharacterWeaponStates NewState);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	ECharacterActionsStates SetAction(ECharacterActionsStates NewAction);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	ECharacterModeStates SetMode(ECharacterModeStates NewForm);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsHumanStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) const;

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsActionEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) const;

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	bool IsModeEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) const;
};