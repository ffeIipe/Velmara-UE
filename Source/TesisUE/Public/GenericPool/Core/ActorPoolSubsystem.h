#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericPool/Interfaces/PoolableActor.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

// Actores inactivos de una misma clase. Es un USTRUCT para que el GC vea las referencias.
USTRUCT()
struct FActorPoolBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Actors;
};

UCLASS()
class TESISUE_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	template <typename T>
	T* SpawnOrRetrieveFromPool(TSubclassOf<T> ClassToSpawn, const FTransform& SpawnTransform)
	{
		return Cast<T>(GetActorFromPool(ClassToSpawn, SpawnTransform));
	}

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReturnToPool(AActor* ActorToReturn);

	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (DeterminesOutputType = "ClassToSpawn", DisplayName = "Spawn Actor From Pool"))
	AActor* GetActorFromPool(const TSubclassOf<AActor>& ClassToSpawn, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void InitializePool(TSubclassOf<AActor> ClassToSpawn, int32 Quantity);

	// Cantidad de actores de esa clase esperando en el pool.
	UFUNCTION(BlueprintPure, Category = "Pooling")
	int32 GetInactiveCount(TSubclassOf<AActor> ActorClass) const;

	// True si el actor está guardado en el pool (inactivo).
	UFUNCTION(BlueprintPure, Category = "Pooling")
	bool IsActorInPool(const AActor* Actor) const;

protected:
	AActor* RetrieveActor(const UClass* ClassType);

private:
	static void ActivatePooledActor(AActor* Actor, const FTransform& SpawnTransform);

	UPROPERTY()
	TMap<UClass*, FGameplayTagContainer> PoolStats;

	UPROPERTY()
	TMap<UClass*, FActorPoolBucket> InactivePools;
};
