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

	static constexpr double ResendActivateInterval = 1.0;

private:
	struct FExposureEntry
	{
		TArray<TWeakObjectPtr<UObject>> Sources;
		double LastActivateTime = 0.0;
	};

	void SendSunEvent(AActor* Target, bool bActivate) const;

	TMap<TWeakObjectPtr<AActor>, FExposureEntry> Exposures;
};
