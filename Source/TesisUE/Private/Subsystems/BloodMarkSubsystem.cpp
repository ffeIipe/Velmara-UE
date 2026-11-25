#include "Subsystems/BloodMarkSubsystem.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GenericTeamAgentInterface.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Entities/Entity.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Kismet/GameplayStatics.h"

void UBloodMarkSubsystem::MarkEnemiesInRange(const FVector& Origin, const float Radius, const float Duration, UMaterialInterface* OverlayMaterial)
{
	const double ExpireTime = GetWorld()->GetTimeSeconds() + Duration;
	const FGameplayTag MarkTag = FVelmaraGameplayTags::Get().State_BloodMarked;
	TArray<TWeakObjectPtr<AActor>> NewlyMarked;

	for (TActorIterator<AEntity> It(GetWorld()); It; ++It)
	{
		AEntity* Entity = *It;
		if (UTotemChargeComponent* Totem = Entity->FindComponentByClass<UTotemChargeComponent>())
		{
			if (FVector::DistSquared(Entity->GetActorLocation(), Origin) <= FMath::Square(Radius))
			{
				Totem->ActivatePossession(OverlayMaterial);
			}
			continue;
		}

		UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent();
		if (!ASC || !IsEnemyOfPlayer(Entity)) continue;
		if (ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) <= 0.f) continue;
		if (FVector::DistSquared(Entity->GetActorLocation(), Origin) > FMath::Square(Radius)) continue;

		if (FMark* Existing = Marks.Find(Entity))
		{
			Existing->ExpireTime = FMath::Max(Existing->ExpireTime, ExpireTime);
			continue;
		}

		FMark& Mark = Marks.Add(Entity);
		Mark.ExpireTime = ExpireTime;
		ASC->SetLooseGameplayTagCount(MarkTag, 1);
		NewlyMarked.Add(Entity);

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

	for (const TWeakObjectPtr<AActor>& Marked : NewlyMarked)
	{
		if (AActor* MarkedActor = Marked.Get())
		{
			OnActorBloodMarked.Broadcast(MarkedActor);
		}
	}
}

void UBloodMarkSubsystem::UpdateMarks()
{
	const double Now = GetWorld()->GetTimeSeconds();
	TArray<TWeakObjectPtr<AActor>> ClearedActors;

	for (auto It = Marks.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Key.Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}

		bool bShouldClear = Now >= It->Value.ExpireTime;
		if (!bShouldClear)
		{
			if (const AEntity* Entity = Cast<AEntity>(Actor))
			{
				const UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent();
				const bool bDead = ASC && ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) <= 0.f;
				bShouldClear = bDead || Entity->IsPlayerControlled();
			}
		}

		if (bShouldClear)
		{
			ClearMark(Actor, It->Value);
			ClearedActors.Add(Actor);
			It.RemoveCurrent();
		}
	}

	if (Marks.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}

	for (const TWeakObjectPtr<AActor>& Cleared : ClearedActors)
	{
		if (AActor* ClearedActor = Cleared.Get())
		{
			OnActorBloodMarkCleared.Broadcast(ClearedActor);
		}
	}
}

bool UBloodMarkSubsystem::IsActorBloodMarked(const AActor* Actor) const
{
	if (!IsValid(Actor)) return false;

	const AEntity* Entity = Cast<AEntity>(Actor);
	if (!Entity || Entity->IsPlayerControlled()) return false;

	const UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent();
	if (ASC && ASC->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) <= 0.f) return false;

	if (Marks.Contains(const_cast<AActor*>(Actor))) return true;

	return ASC && ASC->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_BloodMarked);
}

float UBloodMarkSubsystem::GetRemainingMarkTime(const AActor* Actor) const
{
	if (!Actor || !GetWorld()) return 0.f;

	if (const FMark* Mark = Marks.Find(const_cast<AActor*>(Actor)))
	{
		return static_cast<float>(FMath::Max(0.0, Mark->ExpireTime - GetWorld()->GetTimeSeconds()));
	}
	return 0.f;
}

void UBloodMarkSubsystem::ClearMarkForActor(AActor* Actor)
{
	if (!Actor) return;

	bool bWasMarked = false;
	if (FMark* Mark = Marks.Find(Actor))
	{
		ClearMark(Actor, *Mark);
		Marks.Remove(Actor);
		bWasMarked = true;
	}
	else if (const AEntity* Entity = Cast<AEntity>(Actor))
	{
		if (UAbilitySystemComponent* ASC = Entity->GetAbilitySystemComponent())
		{
			const FGameplayTag MarkTag = FVelmaraGameplayTags::Get().State_BloodMarked;
			if (ASC->HasMatchingGameplayTag(MarkTag))
			{
				ASC->SetLooseGameplayTagCount(MarkTag, 0);
				bWasMarked = true;
			}
		}
	}

	if (Marks.IsEmpty() && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}

	if (bWasMarked)
	{
		OnActorBloodMarkCleared.Broadcast(Actor);
	}
}

int32 UBloodMarkSubsystem::GetMarkedEnemyCount() const
{
	int32 Count = 0;
	for (const TPair<TWeakObjectPtr<AActor>, FMark>& Pair : Marks)
	{
		if (Pair.Key.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

void UBloodMarkSubsystem::GetMarkedActors(TArray<AActor*>& OutActors) const
{
	OutActors.Reset();
	for (const TPair<TWeakObjectPtr<AActor>, FMark>& Pair : Marks)
	{
		if (AActor* Actor = Pair.Key.Get(); Actor && IsActorBloodMarked(Actor))
		{
			OutActors.Add(Actor);
		}
	}
}

void UBloodMarkSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimer);
	}

	for (const TPair<TWeakObjectPtr<AActor>, FMark>& Pair : Marks)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			ClearMark(Actor, Pair.Value);
		}
	}
	Marks.Empty();

	Super::Deinitialize();
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
