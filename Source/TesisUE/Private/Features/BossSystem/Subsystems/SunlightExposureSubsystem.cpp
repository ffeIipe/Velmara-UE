#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"

void USunlightExposureSubsystem::SetExposed(AActor* Target, UObject* Source, const bool bExposed)
{
	if (!Target || !Source) return;

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	if (bExposed)
	{
		FExposureEntry& Entry = Exposures.FindOrAdd(Target);
		Entry.Sources.RemoveAll([](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid(); });

		const bool bWasExposed = Entry.Sources.Num() > 0;
		Entry.Sources.AddUnique(Source);

		if (!bWasExposed || Now - Entry.LastActivateTime >= ResendActivateInterval)
		{
			Entry.LastActivateTime = Now;
			SendSunEvent(Target, true);
		}
		return;
	}

	FExposureEntry* Entry = Exposures.Find(Target);
	if (!Entry) return;

	Entry->Sources.RemoveAll([Source](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Source; });

	if (Entry->Sources.Num() == 0)
	{
		Exposures.Remove(Target);
		SendSunEvent(Target, false);
	}
}

void USunlightExposureSubsystem::ClearSource(UObject* Source)
{
	TArray<AActor*> Targets;
	for (const TPair<TWeakObjectPtr<AActor>, FExposureEntry>& Pair : Exposures)
	{
		if (AActor* Target = Pair.Key.Get())
		{
			Targets.Add(Target);
		}
	}

	for (AActor* Target : Targets)
	{
		SetExposed(Target, Source, false);
	}

	// Limpia objetivos destruidos.
	for (auto It = Exposures.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) It.RemoveCurrent();
	}
}

bool USunlightExposureSubsystem::IsExposed(const AActor* Target) const
{
	const FExposureEntry* Entry = Exposures.Find(Target);
	return Entry && Entry->Sources.Num() > 0;
}

void USunlightExposureSubsystem::SendSunEvent(AActor* Target, const bool bActivate) const
{
	const FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(
		bActivate ? FName("Event.Damage.Sun.Activate") : FName("Event.Damage.Sun.Deactivate"), false);

	if (!EventTag.IsValid()) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = Target;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
}
