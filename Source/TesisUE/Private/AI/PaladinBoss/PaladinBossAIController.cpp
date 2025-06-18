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
    UCharacterStateComponent* PlayerCharStateComp = PlayerPawn ? PlayerPawn->FindComponentByClass<UCharacterStateComponent>() : nullptr;

    if (!BlackboardComponent || !PlayerPawn || !Enemy) return;
    
    if (Stimulus.WasSuccessfullySensed())
    {
        if (PlayerCharStateComp->GetCurrentCharacterState().Form != ECharacterForm::ECF_Possessing)
        {
            BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
            BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), true);
        }
        else
        {
            if (DamageCauser != Actor)
            {
                BlackboardComponent->SetValueAsObject(FName("TargetActor"), DamageCauser);
                BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), true);
            }
            else
            {
                BlackboardComponent->SetValueAsObject(FName("TargetActor"), nullptr);
                BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), false);
                DamageCauser = nullptr;
            }
        }
    }
}