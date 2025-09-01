#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/CharacterHumanStates.h"
#include "CharacterStateComponent.generated.h"

USTRUCT(BlueprintType)
struct TESISUE_API FCharacterStates
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character State")
	ECharacterHumanStates HumanState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Spectral State")
	ECharacterSpectralStates SpectralState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Action")
	ECharacterActions Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character States | Character Form")
	ECharacterMode Mode;
	
	FCharacterStates()
		: HumanState(ECharacterHumanStates::ECHS_Unequipped)
		, SpectralState(ECharacterSpectralStates::ECSS_Unequipped)
		, Action(ECharacterActions::ECA_Nothing)
		, Mode(ECharacterMode::ECM_Human)
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
	ECharacterHumanStates SetHumanState(ECharacterHumanStates NewState);
	
	UFUNCTION(BlueprintCallable, Category = "Character States | Character Spectral State")
	ECharacterSpectralStates SetSpectralState(ECharacterSpectralStates NewSpectralState);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	ECharacterActions SetAction(ECharacterActions NewAction);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	ECharacterMode SetMode(ECharacterMode NewForm);

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsHumanStateEqualToAny(const TArray<ECharacterHumanStates>& StatesToCheck) const;
	
	UFUNCTION(BlueprintCallable, Category = "Character States | Character Spectral State")
	bool IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck) const;

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Action")
	bool IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck) const;

	UFUNCTION(BlueprintCallable, Category = "Character States | Character Form")
	bool IsModeEqualToAny(const TArray<ECharacterMode>& FormsToCheck) const;
};