#include "Items/EnergyItem.h"
#include "Player/PlayerMain.h"
#include "Components/AttributeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/PromptWidgetComponent.h"


void AEnergyItem::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);

	if (bWasUsed) return;

	bWasUsed = true;

	if (NewOwner)
	{
		AttributeProvider->IncreaseEnergy(EnergyToIncrease);
		AttributeProvider->IncreaseHealth(HealthToIncrease);

		if (OnUsedSpawnEnergy.IsBound())
		{
			const int32 EnergyOrbs = FMath::RoundToInt(EnergyToIncrease / 5);

			for (int32 i = 0; i < EnergyOrbs; i++)
			{
				OnUsedSpawnEnergy.Broadcast();
			}
		}

		if (OnUsedSpawnLife.IsBound())
		{
			const int32 LifeOrbs = FMath::RoundToInt(HealthToIncrease / 5);

			for (int32 i = 0; i < LifeOrbs; i++)
			{
				OnUsedSpawnLife.Broadcast();
			}
		}

		if (OpenSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), OpenSFX, ItemMesh->GetComponentLocation());
		}

		if (EnergyToIncrease > 0.f && EnergySFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), EnergySFX, ItemMesh->GetComponentLocation());
		}

		if (HealthToIncrease > 0.f && HealthSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HealthSFX, ItemMesh->GetComponentLocation());
		}
	}
}
