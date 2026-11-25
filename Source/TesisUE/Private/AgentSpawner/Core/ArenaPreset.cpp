#include "AgentSpawner/Core/ArenaPreset.h"

TSubclassOf<AActor> UArenaPreset::PickNextAgentClass(const TArray<TSubclassOf<AActor>>& SpawnedClasses, const TArray<AActor*>& AliveAgents) const
{
	TArray<FArenaAgentEntry> Entries;
	int64 TotalWeight = 0;
	for (const FArenaAgentEntry& Agent : Agents)
	{
		if (!Agent.AgentClass || Agent.Weight <= 0) continue;

		TotalWeight += Agent.Weight;
		if (FArenaAgentEntry* Existing = Entries.FindByPredicate([&Agent](const FArenaAgentEntry& Entry) { return Entry.AgentClass == Agent.AgentClass; }))
		{
			Existing->Weight += Agent.Weight;
			continue;
		}
		Entries.Add(Agent);
	}

	if (TotalWeight == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no agent with a class and a positive weight"), *GetName());
		return nullptr;
	}

	TArray<int32> Quotas;
	TArray<int64> Remainders;
	int32 Unassigned = TotalEnemiesToSpawn;
	for (const FArenaAgentEntry& Entry : Entries)
	{
		const int64 Share = static_cast<int64>(TotalEnemiesToSpawn) * Entry.Weight;
		Quotas.Add(Share / TotalWeight);
		Remainders.Add(Share % TotalWeight);
		Unassigned -= Quotas.Last();
	}

	for (; Unassigned > 0; --Unassigned)
	{
		int32 Largest = 0;
		for (int32 Index = 1; Index < Remainders.Num(); ++Index)
		{
			if (Remainders[Index] > Remainders[Largest]) Largest = Index;
		}
		++Quotas[Largest];
		Remainders[Largest] = -1;
	}

	TArray<int32> Pending;
	int32 TotalPending = 0;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FArenaAgentEntry& Entry = Entries[Index];

		int32 Spawned = 0;
		for (const TSubclassOf<AActor>& SpawnedClass : SpawnedClasses)
		{
			if (SpawnedClass == Entry.AgentClass) ++Spawned;
		}

		int32 Alive = 0;
		for (const AActor* Agent : AliveAgents)
		{
			if (Agent && Agent->GetClass() == Entry.AgentClass) ++Alive;
		}

		const bool bAtAliveLimit = Entry.MaxAliveAtOnce > 0 && Alive >= Entry.MaxAliveAtOnce;
		Pending.Add(bAtAliveLimit ? 0 : FMath::Max(0, Quotas[Index] - Spawned));
		TotalPending += Pending.Last();
	}

	if (TotalPending == 0) return nullptr;

	int32 Roll = FMath::RandRange(0, TotalPending - 1);
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		Roll -= Pending[Index];
		if (Roll < 0) return Entries[Index].AgentClass;
	}

	return nullptr;
}
