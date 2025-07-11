// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 *
 */
UCLASS()
class TESISUE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController(const FObjectInitializer& ObjectInitializer);

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	bool bIsAlreadyCleared = false;

	bool bIsAlreadyFocused = false;

	AActor* DamageCauser;

	void SetHasReservedAttackToken(bool bHasToken);

protected:
	void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAIPerceptionComponent* EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAISenseConfig_Sight* AISenseConfig_Sight;

	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UBlackboardComponent* BlackboardComponent;

	class AEntity* EntityOwner;
	AEntity* CachedTarget;
	
	UPROPERTY(VisibleAnywhere)
	bool bHasReservedAttackToken;

	UPROPERTY(EditAnywhere)
	bool bHasToCheckPlayerForm = true;
};
