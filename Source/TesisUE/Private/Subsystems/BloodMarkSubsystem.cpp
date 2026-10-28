#include "Subsystems/BloodMarkSubsystem.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GenericTeamAgentInterface.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Entities/Entity.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Kismet/GameplayStatics.h"

void UBloodMarkSubsystem::MarkEnemiesInRange(const FVector& Origin, const float Radius, const float Duration, UMaterialInterface* OverlayMaterial)
{
	const double ExpireTime = GetWorld()->GetTimeSeconds() + Duration;
	const FGameplayTag MarkTag = FVelmaraGameplayTags::Get().State_BloodMarked;

	for (TActorIterator<AEntity> It(GetWorld()); It; ++It)
	{
		AEntity* Entity = *It;
		UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent();
		if (!ASC || !IsEnemyOfPlayer(Entity)) continue;
		if (ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) <= 0.f) continue;
		if (FVector::DistSquared(Entity->GetActorLocation(), Origin) > FMath::Square(Radius)) continue;

		if (FMark* Existing = Marks.Find(Entity))
		{
			Existing->ExpireTime = ExpireTime;
			continue;
		}

		FMark& Mark = Marks.Add(Entity);
		Mark.ExpireTime = ExpireTime;
		ASC->SetLooseGameplayTagCount(MarkTag, 1);

		if (USkeletalMeshComponent* Mesh = Entity->GetMesh(); Mesh && OverlayMaterial)
		{
			Mark.Mesh = Mesh;
			Mark.PreviousOverlay = Mesh->GetOverlayMaterial();
			Mesh->SetOverlayMaterial(OverlayMaterial);
		}
	}

	if (!Marks.IsEmpty() && !GetWorld()->GetTimerManager().IsTimerActive(UpdateTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UBloodMarkSubsystem::UpdateMarks, 0.1f, true);
	}
}

void UBloodMarkSubsystem::UpdateMarks()
{
	const double Now = GetWorld()->GetTimeSeconds();

	for (auto It = Marks.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Key.Get();
		if (!Actor)
		{
			It.RemoveCurrent();
		}
		else if (Now >= It->Value.ExpireTime)
		{
			ClearMark(Actor, It->Value);
			It.RemoveCurrent();
		}
	}

	if (Marks.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}
}

void UBloodMarkSubsystem::ClearMark(AActor* Actor, const FMark& Mark) const
{
	if (const AEntity* Entity = Cast<AEntity>(Actor))
	{
		if (UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent())
		{
			ASC->SetLooseGameplayTagCount(FVelmaraGameplayTags::Get().State_BloodMarked, 0);
		}
	}

	if (UMeshComponent* Mesh = Mark.Mesh.Get())
	{
		Mesh->SetOverlayMaterial(Mark.PreviousOverlay.Get());
	}
}

bool UBloodMarkSubsystem::IsEnemyOfPlayer(const AActor* Actor) const
{
	const AEntity* Entity = Cast<AEntity>(Actor);
	if (!Entity || Entity->IsPlayerControlled()) return false;

	const IGenericTeamAgentInterface* PlayerTeamAgent = Cast<IGenericTeamAgentInterface>(UGameplayStatics::GetPlayerController(this, 0));
	return !PlayerTeamAgent || Entity->GetTeamId() != PlayerTeamAgent->GetGenericTeamId();
}
