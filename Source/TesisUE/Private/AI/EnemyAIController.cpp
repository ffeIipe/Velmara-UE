#include "AI/EnemyAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/PossessionComponent.h"
#include "Enemy/Enemy.h"
#include "Subsystems/EnemyTokenManager.h"
#include <Kismet/GameplayStatics.h>

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

    AAIController::SetGenericTeamId(FGenericTeamId(0));
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

void AEnemyAIController::DeactivateController() const
{
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.RemoveAll(this);
}

ETeamAttitude::Type AEnemyAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
    if (!IsDamageCauserValid(Cast<AEntity>(&Other)))
    {
        const APawn* PawnToCheck = Cast<APawn>(&Other);

        if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(PawnToCheck->GetController()))
        {
            FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

            if      (OtherTeamId == FGenericTeamId(0)) return ETeamAttitude::Neutral;
            else if (OtherTeamId == FGenericTeamId(1)) return ETeamAttitude::Friendly;
            else if (OtherTeamId == FGenericTeamId(2)) return ETeamAttitude::Hostile;
        }
    }
    return ETeamAttitude::Neutral;
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    EntityOwner = Cast<AEntity>(GetCharacter());
    BlackboardComponent = GetBlackboardComponent();
    CachedPlayerController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void AEnemyAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!CachedPlayerController) return;
    
    if (EntityOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

    UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();
    if (!TokenManager) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        if (GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile || !bHasToCheckPlayerForm) SetHasAttackToken(TokenManager, Actor); 
        
        if (IsDamageCauserValid(DamageCauser)) SetHasAttackToken(TokenManager, DamageCauser);
    }
    else
    {
        PerceptionComponent->ForgetActor(Actor);
        // BlackboardComponent->ClearValue(FName("TargetActor"));
        if (bHasReservedAttackToken)
        {
            bHasReservedAttackToken = false;
            TokenManager->ReturnAttackToken();
            BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
        }
    }
}

bool AEnemyAIController::IsDamageCauserValid(const AEntity* EntityToCheck) const
{
    return EntityToCheck && (EntityToCheck)->GetPossessionComponent()->IsPossessed() && (EntityToCheck)->GetController() == CachedPlayerController;
}

void AEnemyAIController::SetHasAttackToken(UEnemyTokenManager* TokenManager, AActor* Actor)
{
    if (Actor->IsHidden())
    {
        BlackboardComponent->SetValueAsObject(FName("TargetActor"), nullptr);
        return;
    }
    
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

void AEnemyAIController::SetHasReservedAttackToken(const bool bHasToken)
{
    bHasReservedAttackToken = bHasToken;
    if (!bHasToken && BlackboardComponent)
    {
        BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
    }
}