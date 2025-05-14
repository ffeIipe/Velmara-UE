#include "Items/EnergyItem.h"
#include "Player/PlayerMain.h"
#include "Components/AttributeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/PromptWidgetComponent.h"

void AEnergyItem::Use(ACharacter* TargetCharacter)
{
	if (bWasUsed) return;

	bWasUsed = true;

	Player = Cast<APlayerMain>(TargetCharacter);

	if (Player && Player->GetAttributes())
	{
		Player->GetAttributes()->IncreaseEnergy(EnergyToIncrease);
		Player->GetAttributes()->SetHealth(Player->GetAttributes()->GetHealth() + HealthToIncrease);

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
