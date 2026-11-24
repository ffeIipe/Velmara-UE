#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SunlightExposureSubsystem.generated.h"

UCLASS()
class TESISUE_API USunlightExposureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sunlight")
	void SetExposed(AActor* Target, UObject* Source, bool bExposed);

	UFUNCTION(BlueprintCallable, Category = "Sunlight")
	void ClearSource(UObject* Source);

	UFUNCTION(BlueprintPure, Category = "Sunlight")
	bool IsExposed(const AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Sun")
	void RegisterSunSource(UObject* Source);

	UFUNCTION(BlueprintCallable, Category = "Boss|Sun")
	void UnregisterSunSource(UObject* Source);

	UFUNCTION(BlueprintPure, Category = "Boss|Sun")
	bool HasActiveSunSources() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Sun")
	void SetSunProtection(AActor* Target, bool bProtected);

	UFUNCTION(BlueprintPure, Category = "Boss|Sun")
	bool IsSunProtected(const AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Sun")
	void GrantSunGrace(AActor* Target, float Duration);

	UFUNCTION(BlueprintPure, Category = "Boss|Sun")
	bool IsUnderSunGrace(const AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Sun")
	void ClearActorExposure(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Boss|Sun")
	bool IsPhysicallyInSunlight(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Sun")
	bool IsInShadowDuringSunHazard(const AActor* Target) const;

	static constexpr double ResendActivateInterval = 1.0;

private:
	struct FExposureEntry
	{
		TArray<TWeakObjectPtr<UObject>> Sources;
		double LastActivateTime = 0.0;
	};

	void SendSunEvent(AActor* Target, bool bActivate) const;

	TMap<TWeakObjectPtr<AActor>, FExposureEntry> Exposures;
	TMap<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<UObject>>> RawSunlightSources;
	TSet<TWeakObjectPtr<UObject>> ActiveSunSources;
	TSet<TWeakObjectPtr<AActor>> ProtectedActors;
	TMap<TWeakObjectPtr<AActor>, double> SunGraceUntil;
};
