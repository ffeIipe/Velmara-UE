#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void USunlightExposureSubsystem::SetExposed(AActor* Target, UObject* Source, const bool bExposed)
{
	if (!Target || !Source) return;

	if (bExposed)
	{
		TArray<TWeakObjectPtr<UObject>>& RawSources = RawSunlightSources.FindOrAdd(Target);
		RawSources.RemoveAll([](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid(); });
		RawSources.AddUnique(Source);

		if (IsSunProtected(Target) || IsUnderSunGrace(Target))
		{
			if (FExposureEntry* ExistingEntry = Exposures.Find(Target))
			{
				ExistingEntry->Sources.RemoveAll([Source](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Source; });
				if (ExistingEntry->Sources.Num() == 0)
				{
					Exposures.Remove(Target);
					SendSunEvent(Target, false);
				}
			}
			return;
		}
	}
	else if (TArray<TWeakObjectPtr<UObject>>* RawSources = RawSunlightSources.Find(Target))
	{
		RawSources->RemoveAll([Source](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Source; });
		if (RawSources->Num() == 0)
		{
			RawSunlightSources.Remove(Target);
		}
	}

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
			Targets.AddUnique(Target);
		}
	}

	for (AActor* Target : Targets)
	{
		SetExposed(Target, Source, false);
	}

	for (auto It = Exposures.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = RawSunlightSources.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		It.Value().RemoveAll([Source](const TWeakObjectPtr<UObject>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Source; });
		if (It.Value().Num() == 0)
		{
			It.RemoveCurrent();
		}
	}

	UnregisterSunSource(Source);
}

bool USunlightExposureSubsystem::IsExposed(const AActor* Target) const
{
	if (!Target) return false;

	const FExposureEntry* Entry = Exposures.Find(Target);
	if (!Entry) return false;

	for (const TWeakObjectPtr<UObject>& Src : Entry->Sources)
	{
		if (Src.IsValid()) return true;
	}

	return false;
}

void USunlightExposureSubsystem::RegisterSunSource(UObject* Source)
{
	for (auto It = ActiveSunSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (IsValid(Source))
	{
		ActiveSunSources.Add(Source);
	}
}

void USunlightExposureSubsystem::UnregisterSunSource(UObject* Source)
{
	if (Source)
	{
		ActiveSunSources.Remove(Source);
	}

	for (auto It = ActiveSunSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

bool USunlightExposureSubsystem::HasActiveSunSources() const
{
	for (const TWeakObjectPtr<UObject>& Source : ActiveSunSources)
	{
		if (Source.IsValid()) return true;
	}

	return false;
}

void USunlightExposureSubsystem::SetSunProtection(AActor* Target, const bool bProtected)
{
	if (!Target) return;

	for (auto It = ProtectedActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (bProtected)
	{
		ProtectedActors.Add(Target);
		ClearActorExposure(Target);
	}
	else
	{
		ProtectedActors.Remove(Target);
	}
}

bool USunlightExposureSubsystem::IsSunProtected(const AActor* Target) const
{
	if (!Target) return false;

	return ProtectedActors.Contains(const_cast<AActor*>(Target));
}

void USunlightExposureSubsystem::GrantSunGrace(AActor* Target, const float Duration)
{
	if (!IsValid(Target) || Duration <= 0.f) return;

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const double NewGraceUntil = Now + static_cast<double>(Duration);

	for (auto It = SunGraceUntil.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || It.Value() <= Now)
		{
			It.RemoveCurrent();
		}
	}

	if (double* Existing = SunGraceUntil.Find(Target))
	{
		*Existing = FMath::Max(*Existing, NewGraceUntil);
	}
	else
	{
		SunGraceUntil.Add(Target, NewGraceUntil);
	}

	ClearActorExposure(Target);
}

bool USunlightExposureSubsystem::IsUnderSunGrace(const AActor* Target) const
{
	if (!Target) return false;

	const double* GraceUntil = SunGraceUntil.Find(Target);
	if (!GraceUntil) return false;

	const UWorld* World = GetWorld();
	if (!World) return false;

	return World->GetTimeSeconds() < *GraceUntil;
}

void USunlightExposureSubsystem::ClearActorExposure(AActor* Target)
{
	if (!Target) return;

	Exposures.Remove(Target);
	RawSunlightSources.Remove(Target);

	if (IsValid(Target))
	{
		SendSunEvent(Target, false);
	}
}

bool USunlightExposureSubsystem::IsPhysicallyInSunlight(const AActor* Target) const
{
	if (!Target) return false;

	const TArray<TWeakObjectPtr<UObject>>* RawSources = RawSunlightSources.Find(Target);
	if (!RawSources) return false;

	for (const TWeakObjectPtr<UObject>& Src : *RawSources)
	{
		if (Src.IsValid()) return true;
	}

	return false;
}

bool USunlightExposureSubsystem::IsInShadowDuringSunHazard(const AActor* Target) const
{
	return Target != nullptr && HasActiveSunSources() && !IsPhysicallyInSunlight(Target);
}

void USunlightExposureSubsystem::SendSunEvent(AActor* Target, const bool bActivate) const
{
	if (!Target) return;

	const FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(
		bActivate ? FName("Event.Damage.Sun.Activate") : FName("Event.Damage.Sun.Deactivate"), false);

	if (!EventTag.IsValid()) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Target = Target;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, Payload);
}
