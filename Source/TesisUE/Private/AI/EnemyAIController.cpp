#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/PossessionComponent.h"
#include "Enemy/Enemy.h"
#include "Subsystems/EnemyTokenManager.h"
#include <Player/PlayerHeroController.h>

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

	SetGenericTeamId(FGenericTeamId(0));
}

void AEnemyAIController::CustomInitialize(AEntity* NewOwner, UBlackboardComponent* NewBlackboardComponent, UCharacterStateComponent* NewCharacterStateComponent)
{
    EntityOwner = NewOwner;
    BlackboardComponent = NewBlackboardComponent;
	OwningCharacterStateComponent = NewCharacterStateComponent;

    if (!EnemyPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
    {
        EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AEnemyAIController::OnEnemyPerceptionUpdated);
    }

    PerceptionComponent->ForgetAll();
}

void AEnemyAIController::DeactivateController()
{
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.RemoveAll(this);
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
    const APawn* PawnToCheck = Cast<APawn>(&Other);

    if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(PawnToCheck->GetController()))
    {
        FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

        if      (OtherTeamId == FGenericTeamId(0)) return ETeamAttitude::Neutral;
        else if (OtherTeamId == FGenericTeamId(1)) return ETeamAttitude::Friendly;
        else if (OtherTeamId == FGenericTeamId(2)) return ETeamAttitude::Hostile;
    }
    return ETeamAttitude::Neutral;
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    EntityOwner = Cast<AEntity>(GetCharacter());
    BlackboardComponent = GetBlackboardComponent();
}

void AEnemyAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(449, 3.f, FColor::Blue, FString("Enemy Perception Updated"));

    if (EntityOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

    if (!EntityOwner)
    {
        EntityOwner = Cast<AEntity>(GetCharacter());
    }

    if (!EntityOwner->GetCharacterStateComponent())
    {
        EntityOwner->GetCharacterStateComponent();
    }

    if (!BlackboardComponent)
    {
        BlackboardComponent = GetBlackboardComponent();
    }

    if (Stimulus.WasSuccessfullySensed()) //TODO: en el combat componenent agregar una especie de team attitude custom
    {
        UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();

        if (GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
        {
            SetHasAttackToken(TokenManager, Actor);
        }
        else if (DamageCauser != Actor)
        {
			SetHasAttackToken(TokenManager, DamageCauser);
        }
        else
        {
            PerceptionComponent->ForgetActor(Actor);
            BlackboardComponent->ClearValue(FName("TargetActor"));

            if (bHasReservedAttackToken)
            {
                bHasReservedAttackToken = false;
                TokenManager->ReturnAttackToken();
                BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
            }
        }
    }
    else
    {
        UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();
        
        if (bHasReservedAttackToken)
        {
            bHasReservedAttackToken = false;
            TokenManager->ReturnAttackToken();
            BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
        }
    }
}

void AEnemyAIController::SetHasAttackToken(UEnemyTokenManager* TokenManager, AActor* Actor)
{
    if (!bHasReservedAttackToken)
    {
        if (TokenManager && TokenManager->TryReserveAttackToken())
        {
            bHasReservedAttackToken = true;
            BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), true);
        }
        else
        {
            BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
        }
    }
    else
    {
        BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), true);
    }

    BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
}

void AEnemyAIController::SetHasReservedAttackToken(bool bHasToken)
{
    bHasReservedAttackToken = bHasToken;
    if (!bHasToken && BlackboardComponent)
    {
        BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
    }
}