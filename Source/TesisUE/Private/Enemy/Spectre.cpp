#include "Enemy/Spectre.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/SpectralTrapDamageType.h"


float ASpectre::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		if (Attributes)
		{
			if (Attributes->IsAlive())
			{
				Attributes->ReceiveDamage(DamageAmount);
			}
			else Die(DamageCauser);
		}
	}
	return DamageAmount;
}

void ASpectre::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType)
{
	if (Attributes && Attributes->IsAlive())
	{
		//TODO: here put the logic when the spectre receives a hit
	}
	else
	{
		Die(DamageCauserOf);
	}

	Super::GetHit_Implementation(ImpactPoint, UDamageType::StaticClass());
}