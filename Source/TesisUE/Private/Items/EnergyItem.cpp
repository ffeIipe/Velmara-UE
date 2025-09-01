#include "Items/EnergyItem.h"
#include "Player/PlayerMain.h"
#include "Components/AttributeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/PromptWidgetComponent.h"

void AEnergyItem::Use(ACharacter* TargetCharacter)
{
	if (bWasUsed) return;

	bWasUsed = true;

	AEntity* UserEntity = Cast<AEntity>(TargetCharacter);

	if (UserEntity && UserEntity->GetAttributeComponent())
	{
		UserEntity->GetAttributeComponent()->IncreaseEnergy(EnergyToIncrease);

		UserEntity->GetAttributeComponent()->SetHealth(UserEntity->GetAttributeComponent()->GetHealth() + HealthToIncrease);

		if (OnUsedSpawnEnergy.IsBound())
		{
			int32 EnergyOrbs = FMath::RoundToInt(EnergyToIncrease / 5);

			for (int32 i = 0; i < EnergyOrbs; i++)
			{
				OnUsedSpawnEnergy.Broadcast();
			}
		}

		if (OnUsedSpawnLife.IsBound())
		{
			int32 LifeOrbs = FMath::RoundToInt((HealthToIncrease) / 5);

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

		PromptWidget->EnablePromptWidget(false);
	}
}
