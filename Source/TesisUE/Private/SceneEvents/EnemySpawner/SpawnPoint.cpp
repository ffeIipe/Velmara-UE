#include "SceneEvents/EnemySpawner/SpawnPoint.h"
#include "Entities/Entity.h"


// Sets default values
ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASpawnPoint::Spawn() const
{
	if (EntityClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        	
		GetWorld()->SpawnActor<AEntity>(EntityClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	}
}