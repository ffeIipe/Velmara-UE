#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class ICombatTargetInterface;
class IOwnerUtilsInterface;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class APlayerController;
class AEntity;

UCLASS()
class TESISUE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController(const FObjectInitializer& ObjectInitializer);

	void CustomInitialize(const class AEntity* NewOwner, UBlackboardComponent* NewBlackboardComponent, class UCharacterStateComponent* NewCharacterStateComponent);

	void DeactivateController() const;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	bool bIsAlreadyCleared = false;

	bool bIsAlreadyFocused = false;

	UPROPERTY()
	TScriptInterface<ICombatTargetInterface> DamageCauser;

	void SetHasReservedAttackToken(bool bHasToken);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAIPerceptionComponent* EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAISenseConfig_Sight* AISenseConfig_Sight;

	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool IsDamageCauserValid(const TScriptInterface<ICombatTargetInterface>& EntityToCheck) const;

	void SetHasAttackToken(class UEnemyTokenManager* TokenManager, TScriptInterface<ICombatTargetInterface> CombatTarget);

	UPROPERTY()
	UBlackboardComponent* BlackboardComponent;

	UPROPERTY()
	TScriptInterface<IOwnerUtilsInterface> EntityOwner;

	UPROPERTY()
	AEntity* CachedTarget;

	UPROPERTY()
	UCharacterStateComponent* OwningCharacterStateComponent;

	UPROPERTY(VisibleAnywhere)
	bool bHasReservedAttackToken;

	UPROPERTY(EditAnywhere)
	bool bHasToCheckPlayerForm = true;

	UPROPERTY()
	APlayerController* CachedPlayerController = nullptr;
};
