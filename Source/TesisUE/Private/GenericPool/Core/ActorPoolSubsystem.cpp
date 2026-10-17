#include "GenericPool/Core/ActorPoolSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"

namespace ActorPool
{
	// Sin colisión, un Character con el movimiento activo cae por el piso hasta el KillZ y se destruye estando en el pool.
	void SetMovementActive(const AActor* Actor, const bool bActive)
	{
		TInlineComponentArray<UMovementComponent*> MovementComponents(Actor);
		for (UMovementComponent* Movement : MovementComponents)
		{
			if (bActive)
			{
				if (Movement->bAutoActivate)
				{
					Movement->Activate(true);
				}
			}
			else
			{
				Movement->StopMovementImmediately();
				Movement->Deactivate();
			}
		}
	}
}

void UActorPoolSubsystem::Deinitialize()
{
	InactivePools.Empty();
	Super::Deinitialize();
}

void UActorPoolSubsystem::ReturnToPool(AActor* ActorToReturn)
{
	if (!IsValid(ActorToReturn)) return;

	FActorPoolBucket& Bucket = InactivePools.FindOrAdd(ActorToReturn->GetClass());

	// Devolver dos veces el mismo actor haría que dos pedidos posteriores reciban la misma instancia.
	if (Bucket.Actors.Contains(ActorToReturn)) return;

	if (ActorToReturn->Implements<UPoolableActor>())
	{
		IPoolableActor::Execute_OnPoolDeactivate(ActorToReturn);
	}

	ActorToReturn->SetActorHiddenInGame(true);
	ActorToReturn->SetActorEnableCollision(false);
	ActorToReturn->SetActorTickEnabled(false);
	ActorPool::SetMovementActive(ActorToReturn, false);

	Bucket.Actors.Push(ActorToReturn);
}

AActor* UActorPoolSubsystem::GetActorFromPool(const TSubclassOf<AActor>& ClassToSpawn, const FTransform& SpawnTransform)
{
	if (!ClassToSpawn) return nullptr;

	if (AActor* Recycled = RetrieveActor(ClassToSpawn))
	{
		ActivatePooledActor(Recycled, SpawnTransform);
		return Recycled;
	}

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewActor = World->SpawnActor<AActor>(ClassToSpawn, SpawnTransform, SpawnParams);
	if (NewActor && NewActor->Implements<UPoolableActor>())
	{
		IPoolableActor::Execute_OnPoolActivate(NewActor);
	}

	return NewActor;
}

void UActorPoolSubsystem::InitializePool(const TSubclassOf<AActor> ClassToSpawn, const int32 Quantity)
{
	if (!ClassToSpawn || Quantity <= 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	InactivePools.FindOrAdd(ClassToSpawn).Actors.Reserve(GetInactiveCount(ClassToSpawn) + Quantity);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < Quantity; ++i)
	{
		if (AActor* NewActor = World->SpawnActor<AActor>(ClassToSpawn, FTransform::Identity, SpawnParams); IsValid(NewActor))
		{
			ReturnToPool(NewActor);
		}
	}
}

int32 UActorPoolSubsystem::GetInactiveCount(const TSubclassOf<AActor> ActorClass) const
{
	const FActorPoolBucket* Bucket = InactivePools.Find(ActorClass.Get());
	return Bucket ? Bucket->Actors.Num() : 0;
}

bool UActorPoolSubsystem::IsActorInPool(const AActor* Actor) const
{
	if (!Actor) return false;

	const FActorPoolBucket* Bucket = InactivePools.Find(Actor->GetClass());
	return Bucket && Bucket->Actors.Contains(Actor);
}

AActor* UActorPoolSubsystem::RetrieveActor(const UClass* ClassType)
{
	FActorPoolBucket* Bucket = InactivePools.Find(ClassType);
	if (!Bucket) return nullptr;

	while (Bucket->Actors.Num() > 0)
	{
		// Los actores destruidos por fuera del pool (ej. streaming) quedan inválidos y se descartan.
		if (AActor* Candidate = Bucket->Actors.Pop(EAllowShrinking::No); IsValid(Candidate))
		{
			return Candidate;
		}
	}

	return nullptr;
}

void UActorPoolSubsystem::ActivatePooledActor(AActor* Actor, const FTransform& SpawnTransform)
{
	Actor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	Actor->SetActorHiddenInGame(false);
	Actor->SetActorEnableCollision(true);
	Actor->SetActorTickEnabled(true);
	ActorPool::SetMovementActive(Actor, true);

	if (Actor->Implements<UPoolableActor>())
	{
		IPoolableActor::Execute_OnPoolActivate(Actor);
	}
}
