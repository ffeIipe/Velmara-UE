// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Enemy/Enemy.h"

AEnemyAIController::AEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{
	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>("EnemySenseConfig_Sight");
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;
	AISenseConfig_Sight->SightRadius = 5000.f;
	AISenseConfig_Sight->LoseSightRadius = 0.f;
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 100.f;

	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnEnemyPerceptionUpdated);

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
	if (bPauseEnemyPerceptionUpdate) return;

	if (!Actor) return;

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent) return;

	APawn* PlayerPawn = Cast<APawn>(Actor);
	if (!PlayerPawn) return;

	UCharacterStateComponent* CharacterStateComponent = PlayerPawn->FindComponentByClass<UCharacterStateComponent>();

	AEnemy* Enemy = Cast<AEnemy>(GetPawn());

	if (!Enemy) return;

	if (Enemy->GetEnemyType() == EEnemyType::Paladin && CharacterStateComponent && CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
	{
		BlackboardComponent->ClearValue(FName("TargetActor"));
		BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), false);
		StopMovement();
		return;
	}
	
	if (Stimulus.WasSuccessfullySensed() && CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
	{
		BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
		BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), true);
	}
}	