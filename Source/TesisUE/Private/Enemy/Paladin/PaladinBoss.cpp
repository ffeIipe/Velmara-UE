

#include "Enemy/Paladin/PaladinBoss.h"

APaladinBoss::APaladinBoss()
{
	USceneComponent* SP1 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint1"));
	if (SP1)
	{
		SP1->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP1);
	}
	USceneComponent* SP2 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint2"));
	if (SP2)
	{
		SP2->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP2);
	}
	USceneComponent* SP3 = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint3"));
	if (SP3)
	{
		SP3->SetupAttachment(GetRootComponent());
		SpawnPoints.Add(SP3);
	}
}

void APaladinBoss::BeginPlay()
{

}

void APaladinBoss::Invoke()
{
	for (USceneComponent* SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = SpawnPoint->GetComponentLocation();
			FRotator SpawnRotation = SpawnPoint->GetComponentRotation();

			APaladin* NewPaladin = GetWorld()->SpawnActor<APaladin>(APaladin::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);

			NewPaladin->OnDestroyed.AddDynamic(this, &APaladinBoss::RemoveMinion(NewPaladin));
			Minions.Add(NewPaladin);
		}
	}
}

void APaladinBoss::RemoveMinion(APaladin* PaladinToRemove)
{

}