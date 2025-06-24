// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Enemy/Enemy.h"

AEnemyAIController::AEnemyAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{
	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>("EnemySenseConfig_Sight");
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;
    AISenseConfig_Sight->AutoSuccessRangeFromLastSeenLocation = 2000.f;
    AISenseConfig_Sight->SetMaxAge(.1f);

	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AEnemyAIController::OnEnemyPerceptionUpdated);

	SetGenericTeamId(FGenericTeamId(1));
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(PawnToCheck->GetController());

	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() < GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Friendly;
}

void AEnemyAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    AEnemy* Enemy = Cast<AEnemy>(GetPawn());
    APawn* PlayerPawn = Cast<APawn>(Actor);
    UCharacterStateComponent* CharacterStateComponent = PlayerPawn ? PlayerPawn->FindComponentByClass<UCharacterStateComponent>() : nullptr; // Añadir null check
    UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

    if (!BlackboardComponent || !PlayerPawn || !Enemy) return;

    if (!CharacterStateComponent)
    {
        BlackboardComponent->ClearValue(FName("TargetActor"));
        BlackboardComponent->ClearValue(FName("DistToTarget"));
        BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), false);

        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
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