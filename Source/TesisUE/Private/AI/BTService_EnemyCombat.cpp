#include "AI/BTService_EnemyCombat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AI/Core/CombatDirectorSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GAS/VelmaraAttributeSet.h"

TAutoConsoleVariable<bool> CVarDebugEnemyCombat(TEXT("AI.DebugEnemyCombat"), false, TEXT("Loguea las decisiones de combate de los enemigos (servicio y ataque)."));

UBTService_EnemyCombat::UBTService_EnemyCombat()
{
	NodeName = TEXT("Enemy Combat Decision");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	CombatStateKey.SelectedKeyName = TEXT("CombatState");
}

void UBTService_EnemyCombat::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		CombatStateKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_EnemyCombat::GetStaticDescription() const
{
	const FString Mode = bRanged
		? FString::Printf(TEXT("Ranged: cast %.0f-%.0f"), MinCastRange, MaxCastRange)
		: FString::Printf(TEXT("Melee: engage %.0f, abort %.0f"), EngageRange, AbortRange);
	return FString::Printf(TEXT("%s\n%s"), *Mode, *Super::GetStaticDescription());
}

void UBTService_EnemyCombat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Blackboard || !Pawn) return;

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	const bool bLostInterest = Target && LoseInterestRange > 0.f && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) > LoseInterestRange;
	if (Target && (bLostInterest || !IsTargetAlive(Target)))
	{
		ReleaseToken(OwnerComp);
		Blackboard->ClearValue(TargetActorKey.SelectedKeyName);
		OwnerComp.GetAIOwner()->ClearFocus(EAIFocusPriority::Gameplay);
		Target = nullptr;
	}
	if (!Target)
	{
		if (UCombatDirectorSubsystem* Director = Pawn->GetWorld()->GetSubsystem<UCombatDirectorSubsystem>())
		{
			Director->ReturnAllAttackTokens(Pawn);
		}
		return;
	}

	const float Distance = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());
	bool bAttack;

	if (bRanged)
	{
		bAttack = Distance >= MinCastRange && Distance <= MaxCastRange && (!bRequireLineOfSight || HasLineOfSight(Pawn, Target));
	}
	else
	{
		UCombatDirectorSubsystem* Director = Pawn->GetWorld()->GetSubsystem<UCombatDirectorSubsystem>();
		const bool bHasToken = Director && Director->HasAttackToken(Pawn, Target);

		if (bHasToken && Distance > AbortRange)
		{
			Director->ReturnAttackToken(Pawn, Target);
			bAttack = false;
		}
		else if (bHasToken)
		{
			bAttack = true;
		}
		else
		{
			bAttack = Director && Distance <= EngageRange && Director->RequestAttackToken(Pawn, Target);
		}
	}

	Blackboard->SetValueAsEnum(CombatStateKey.SelectedKeyName, bAttack ? AttackStateValue : RepositionStateValue);

	UE_CLOG(CVarDebugEnemyCombat.GetValueOnGameThread(), LogTemp, Log, TEXT("EnemyCombat %s: dist %.0f -> %s"), *Pawn->GetName(), Distance,
		bAttack ? TEXT("Attack") : TEXT("Reposition"));
}

void UBTService_EnemyCombat::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_CLOG(CVarDebugEnemyCombat.GetValueOnGameThread(), LogTemp, Log, TEXT("EnemyCombat %s: cease relevant"),
		*GetNameSafe(OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr));
	ReleaseToken(OwnerComp);

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

bool UBTService_EnemyCombat::IsTargetAlive(const AActor* Target)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Target));
	if (!ASC || !ASC->HasAttributeSetForAttribute(UVelmaraAttributeSet::GetHealthAttribute())) return IsValid(Target);

	return ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) > 0.f;
}

bool UBTService_EnemyCombat::HasLineOfSight(const APawn* Pawn, const AActor* Target) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyCombatLOS), false, Pawn);
	Params.AddIgnoredActor(Target);

	const FVector Start = Pawn->GetPawnViewLocation();
	const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	return !Pawn->GetWorld()->LineTraceTestByChannel(Start, End, ECC_Visibility, Params);
}

void UBTService_EnemyCombat::ReleaseToken(UBehaviorTreeComponent& OwnerComp) const
{
	if (bRanged) return;

	const AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn) return;

	if (UCombatDirectorSubsystem* Director = Pawn->GetWorld()->GetSubsystem<UCombatDirectorSubsystem>())
	{
		Director->ReturnAllAttackTokens(Pawn);
	}
}
