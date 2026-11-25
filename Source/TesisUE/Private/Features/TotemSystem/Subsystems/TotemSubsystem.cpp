#include "Features/TotemSystem/Subsystems/TotemSubsystem.h"

#include "Features/TotemSystem/Core/TotemChargeComponent.h"

void UTotemSubsystem::RegisterTotem(UTotemChargeComponent* Totem)
{
	if (Totem)
	{
		Totems.AddUnique(Totem);
	}
}

void UTotemSubsystem::UnregisterTotem(UTotemChargeComponent* Totem)
{
	Totems.Remove(Totem);
}

void UTotemSubsystem::NotifyEntityDied(AActor* DeadActor)
{
	if (!DeadActor) return;

	Totems.RemoveAll([](const TWeakObjectPtr<UTotemChargeComponent>& Totem) { return !Totem.IsValid(); });

	const TArray<TWeakObjectPtr<UTotemChargeComponent>> TotemsSnapshot = Totems;

	for (const TWeakObjectPtr<UTotemChargeComponent>& Totem : TotemsSnapshot)
	{
		if (Totem.IsValid())
		{
			Totem->TryAbsorbDeath(DeadActor);
		}
	}
}

void UTotemSubsystem::GetRegisteredTotems(TArray<UTotemChargeComponent*>& OutTotems) const
{
	OutTotems.Reset();
	for (const TWeakObjectPtr<UTotemChargeComponent>& Totem : Totems)
	{
		if (UTotemChargeComponent* Comp = Totem.Get())
		{
			OutTotems.Add(Comp);
		}
	}
}
