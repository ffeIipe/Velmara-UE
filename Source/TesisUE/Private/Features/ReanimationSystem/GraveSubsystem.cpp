#include "Features/ReanimationSystem/GraveSubsystem.h"

#include "Engine/World.h"
#include "Features/BossSystem/Core/BossEnemy.h"
#include "GameFramework/Pawn.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"

namespace Graves
{
	// Nadie revive tumbas más viejas que esto, así que no vale la pena guardarlas.
	constexpr double MaxKeptAge = 120.0;
}

void UGraveSubsystem::RecordDeath(AActor* DeadActor)
{
	if (!DeadActor || DeadActor->IsA<ABossEnemy>()) return;
	if (const APawn* Pawn = Cast<APawn>(DeadActor); Pawn && Pawn->IsPlayerControlled()) return;

	const double Now = GetWorld()->GetTimeSeconds();
	Graves.RemoveAll([Now](const FEntityGrave& Grave) { return Now - Grave.DeathTime > Graves::MaxKeptAge; });

	FEntityGrave& Grave = Graves.AddDefaulted_GetRef();
	Grave.Corpse = DeadActor;
	Grave.EntityClass = DeadActor->GetClass();
	Grave.Location = DeadActor->GetActorLocation();
	Grave.DeathTime = Now;
}

bool UGraveSubsystem::IsReady(const FEntityGrave& Grave, const FVector& Center, const float Radius, const float MaxAge) const
{
	if (!Grave.EntityClass) return false;
	if (GetWorld()->GetTimeSeconds() - Grave.DeathTime > MaxAge) return false;
	if (FVector::DistSquared(Grave.Location, Center) > FMath::Square(Radius)) return false;

	// El cadáver tiene que haberse ido (al pool o destruido).
	const AActor* Corpse = Grave.Corpse.Get();
	const UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	return !IsValid(Corpse) || (Pool && Pool->IsActorInPool(Corpse));
}

bool UGraveSubsystem::HasReadyGrave(const FVector& Center, const float Radius, const float MaxAge) const
{
	return Graves.ContainsByPredicate([&](const FEntityGrave& Grave) { return IsReady(Grave, Center, Radius, MaxAge); });
}

TArray<FEntityGrave> UGraveSubsystem::ClaimGraves(const FVector& Center, const float Radius, const float MaxAge, const int32 MaxCount)
{
	TArray<int32> ReadyIndices;
	for (int32 Index = 0; Index < Graves.Num(); ++Index)
	{
		if (IsReady(Graves[Index], Center, Radius, MaxAge)) ReadyIndices.Add(Index);
	}

	ReadyIndices.Sort([&](const int32 A, const int32 B)
	{
		return FVector::DistSquared(Graves[A].Location, Center) < FVector::DistSquared(Graves[B].Location, Center);
	});
	ReadyIndices.SetNum(FMath::Min(ReadyIndices.Num(), MaxCount));

	TArray<FEntityGrave> Claimed;
	for (const int32 Index : ReadyIndices)
	{
		Claimed.Add(Graves[Index]);
	}

	// De atrás para adelante para que los índices sigan valiendo.
	ReadyIndices.Sort([](const int32 A, const int32 B) { return A > B; });
	for (const int32 Index : ReadyIndices)
	{
		Graves.RemoveAt(Index);
	}

	return Claimed;
}
