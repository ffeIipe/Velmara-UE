#include "AI/PaladinBoss/PaladinBossAIController.h"
#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Enemy/Enemy.h"

void APaladinBossAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    AEnemy* Enemy = Cast<AEnemy>(GetPawn());
    APawn* PlayerPawn = Cast<APawn>(Actor);
    UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

    if (!BlackboardComponent || !PlayerPawn || !Enemy || bPauseEnemyPerceptionUpdate) return;
    
    if (Stimulus.WasSuccessfullySensed())
    {
        BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
        BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), true);
    }
}