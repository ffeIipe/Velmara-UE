#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CharacterStates.h"
#include "CharacterStateComponent.generated.h"

USTRUCT(BlueprintType)
struct TESISUE_API FCharacterStates
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character State")
	ECharacterStates State;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Spectral State")
	ECharacterSpectralStates SpectralState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Action")
	ECharacterActions Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Form")
	ECharacterForm Form;

	FCharacterStates()
		: State(ECharacterStates::ECS_Unequipped)
		, SpectralState(ECharacterSpectralStates::ECSS_Unequipped)
		, Action(ECharacterActions::ECA_Nothing)
		, Form(ECharacterForm::ECF_Human)
	{
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UCharacterStateComponent : public UActorComponent
{
	GENERATED_BODY()	

public:
	UCharacterStateComponent();

	UFUNCTION(BlueprintCallable, Category = "States")
	const FCharacterStates& GetCurrentCharacterState();

	UFUNCTION(BlueprintCallable, Category = "Character States | Character State")
	ECharacterStates SetCharacterState(ECharacterStates NewState);
	
	UFUNCTION(BlueprintCallable, Category = "Character States | Character Spectral State")
	ECharacterSpectralStates SetCharacterSpectralState(ECharacterSpectralStates NewSpectralState);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	ECharacterActions SetCharacterAction(ECharacterActions NewAction);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	ECharacterForm SetCharacterForm(ECharacterForm NewForm);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck);
	
	UFUNCTION(BlueprintCallable, Category = "Character States | Character Spectral State")
	bool IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	bool IsFormEqualToAny(const TArray<ECharacterForm>& FormsToCheck);

protected:
	virtual void BeginPlay() override;

private:
	FCharacterStates CharacterStates;
};