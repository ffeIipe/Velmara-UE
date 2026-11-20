#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GraveSubsystem.generated.h"

USTRUCT()
struct FEntityGrave
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> Corpse;

	UPROPERTY()
	TSubclassOf<AActor> EntityClass;

	FVector Location = FVector::ZeroVector;

	double DeathTime = 0.0;
};

UCLASS()
class TESISUE_API UGraveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RecordDeath(AActor* DeadActor);

	bool HasReadyGrave(const FVector& Center, float Radius, float MaxAge) const;

	TArray<FEntityGrave> ClaimGraves(const FVector& Center, float Radius, float MaxAge, int32 MaxCount);

private:
	bool IsReady(const FEntityGrave& Grave, const FVector& Center, float Radius, float MaxAge) const;

	UPROPERTY()
	TArray<FEntityGrave> Graves;
};
