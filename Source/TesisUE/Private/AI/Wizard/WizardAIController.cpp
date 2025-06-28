// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Wizard/WizardAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Enemy/Enemy.h"

void AWizardAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    AEnemy* Enemy = Cast<AEnemy>(GetPawn());
    APawn* PlayerPawn = Cast<APawn>(Actor);
    UCharacterStateComponent* CharacterStateComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UCharacterStateComponent>() : nullptr;
    UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

    if (!BlackboardComponent || !PlayerPawn || !Enemy || Enemy->CharacterStateComponent->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;
    

    if (!CharacterStateComponent)
    {
        BlackboardComponent->ClearValue(FName("TargetActor"));
        BlackboardComponent->ClearValue(FName("DistToTarget"));
        BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), false);

        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {

        BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
        BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), true);
    }
}
