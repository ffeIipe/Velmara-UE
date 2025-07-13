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

void AEnemyAIController::CustomInitialize(AEntity* NewOwner, UBlackboardComponent* NewBlackboardComponent, UCharacterStateComponent* NewCharacterStateComponent)
{
    EntityOwner = NewOwner;
    BlackboardComponent = NewBlackboardComponent;
	OwningCharacterStateComponent = NewCharacterStateComponent;

    if (!EnemyPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
    {
        EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AEnemyAIController::OnEnemyPerceptionUpdated);
    }   

    if (IsValid(EntityOwner) 
        && IsValid(BlackboardComponent) 
        && IsValid(OwningCharacterStateComponent) 
        && EnemyPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
    {
	    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Controller initialized..."));
    }
    else if (GEngine)     
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Controller initialization failed!"));
	}
}

void AEnemyAIController::DeactivateController()
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, FString("Controller deactivated..."));
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AEnemyAIController::OnEnemyPerceptionUpdated);
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

    EntityOwner = Cast<AEntity>(GetCharacter());
    BlackboardComponent = GetBlackboardComponent();

    if (EntityOwner && BlackboardComponent)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Entity Owner & BBComponent are valids! And Begin Play was executed..."));
	}

    if (!EnemyPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, FString("Enemy perception update it´s not bound!!!"));
    }
}

void AEnemyAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (GEngine) GEngine->AddOnScreenDebugMessage(449, 3.f, FColor::Blue, FString("Enemy Perception Updated"));

    if (!EntityOwner)
    {
        EntityOwner = Cast<AEntity>(GetCharacter());
    }

    if (!EntityOwner->GetCharacterStateComponent())
    {
        if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Red, FString("Invalid Character State Component..."));
        return;
    }

    if (!BlackboardComponent)
    {
        BlackboardComponent = GetBlackboardComponent();
    }

    if (EntityOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

    AEntity* PlayerPawn = Cast<AEntity>(Actor);
    UCharacterStateComponent* CharacterStateComponent = PlayerPawn ? PlayerPawn->GetCharacterStateComponent() : nullptr;

    if (!CharacterStateComponent)
    {
        BlackboardComponent->ClearValue(FName("TargetActor"));
        BlackboardComponent->ClearValue(FName("DistToTarget"));

        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human || !bHasToCheckPlayerForm)
        {
            if (!bHasReservedAttackToken)
            {
                UEnemyTokenManager* TokenManager = GetWorld()->GetSubsystem<UEnemyTokenManager>();

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
        else
        {
            if (DamageCauser != Actor)
            {
                BlackboardComponent->SetValueAsObject(FName("TargetActor"), DamageCauser);
            }
            else
            {
                BlackboardComponent->SetValueAsObject(FName("TargetActor"), nullptr);
                DamageCauser = nullptr;
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
        }
    }
}

void AEnemyAIController::SetHasReservedAttackToken(bool bHasToken)
{
    bHasReservedAttackToken = bHasToken;
    if (!bHasToken && BlackboardComponent)
    {
        BlackboardComponent->SetValueAsBool(FName("HasAttackToken"), false);
    }
}