#include "Enemy/Spectre.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

void ASpectre::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (Attributes && Attributes->IsAlive())
	{
		//TODO: here put the logic when the spectre receives a hit
	}
	else
	{
		Die();
	}

	Super::GetHit_Implementation(ImpactPoint);
}

void ASpectre::Die()
{
	Destroy();
}
