// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Enemy/Enemy.h"
#include "Subsystems/EnemyTokenManager.h"

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

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    CachedOwner = Cast<AEntity>(GetPawn());
    BlackboardComponent = GetBlackboardComponent();
}

void AEnemyAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(449, 3.f, FColor::Blue, FString("Enemy Perception Updated"));

    AEntity* PlayerPawn = Cast<AEntity>(Actor);
    UCharacterStateComponent* CharacterStateComponent = PlayerPawn ? PlayerPawn->GetCharacterStateComponent() : nullptr; // Añadir null check

    if (!CachedOwner->GetCharacterStateComponent())
    {
        if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Red, FString("Invalid Character State Component..."));
    }

    if (!BlackboardComponent || !PlayerPawn || !CachedOwner || !CachedOwner->GetCharacterStateComponent()) return;
   
    if (CachedOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

    if (!CharacterStateComponent)
    {
        BlackboardComponent->ClearValue(FName("TargetActor"));
        BlackboardComponent->ClearValue(FName("DistToTarget"));
        BlackboardComponent->SetValueAsBool(FName("CanSeePlayer"), false);

        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        /*if (GEngine) GEngine->AddOnScreenDebugMessage(440, 3.f, FColor::White, FString("Sensing..."));*/

        if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human)
        {
            if (!bHasReservedAttackToken)
            {
                UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();

                /*if (GEngine) GEngine->AddOnScreenDebugMessage(442, 3.f, FColor::Purple, FString("Sensed and i do not have a token..."));*/

                if (TokenManager && TokenManager->TryReserveAttackToken())
                {
                    /*if (GEngine) GEngine->AddOnScreenDebugMessage(444, 3.f, FColor::Green, FString("Sensed an i am reserving a token..."));*/
                    bHasReservedAttackToken = true;
                    BlackboardComponent->SetValueAsBool(FName("CanPerformMelee"), true);
                }
                else
                {
                    /*if (GEngine) GEngine->AddOnScreenDebugMessage(443, 3.f, FColor::Orange, FString("Sensed but cannot reserve a token..."));*/
                    BlackboardComponent->SetValueAsBool(FName("CanPerformMelee"), false);
                }
            }
            else
            {
               /*if (GEngine) GEngine->AddOnScreenDebugMessage(441, 3.f, FColor::Magenta, FString("Sensed and i have a token..."));*/
                BlackboardComponent->SetValueAsBool(FName("CanPerformMelee"), true);
            }

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
    else
    {
        /*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Not sensed..."));*/
        UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();
        
        if (bHasReservedAttackToken)
        {
            bHasReservedAttackToken = false;
            TokenManager->ReturnAttackToken();
        }
    }
}

void AEnemyAIController::SetHasReservedAttackToken(bool bHasToken)
{
    bHasReservedAttackToken = bHasToken;
    if (!bHasToken && BlackboardComponent)
    {
        BlackboardComponent->SetValueAsBool(FName("CanPerformMelee"), false);
    }
}