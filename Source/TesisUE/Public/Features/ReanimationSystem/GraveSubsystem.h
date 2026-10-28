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

/*
 * Recuerda dónde murió cada enemigo para que el SkeletonMage pueda levantarlo. Una tumba queda lista recién cuando
 * el cadáver volvió al pool: así no hay dos cuerpos a la vez ni el "volver al pool" pendiente de CBP_Enemy se lleva al revivido.
 */
UCLASS()
class TESISUE_API UGraveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// AEntity lo llama al morir. Ignora jugadores y bosses.
	void RecordDeath(AActor* DeadActor);

	bool HasReadyGrave(const FVector& Center, float Radius, float MaxAge) const;

	// Saca del registro hasta MaxCount tumbas listas, las más cercanas primero.
	TArray<FEntityGrave> ClaimGraves(const FVector& Center, float Radius, float MaxAge, int32 MaxCount);

private:
	bool IsReady(const FEntityGrave& Grave, const FVector& Center, float Radius, float MaxAge) const;

	UPROPERTY()
	TArray<FEntityGrave> Graves;
};
