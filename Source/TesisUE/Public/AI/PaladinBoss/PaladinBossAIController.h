// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyAIController.h"
#include "PaladinBossAIController.generated.h"

/**
 * 
 */
UCLASS()
class TESISUE_API APaladinBossAIController : public AEnemyAIController
{
	GENERATED_BODY()

	void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus) override;
};
