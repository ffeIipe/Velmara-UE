#include "Items/SpectralWeaponItem.h"
#include "Player/PlayerMain.h"
#include "Components/SpectralWeaponComponent.h"
#include "Components/CharacterStateComponent.h"

void ASpectralWeaponItem::Use(ACharacter* Character)
{
	Player = Cast<APlayerMain>(Character);

	if (Player && Player->CharacterStateComponent)
	{
		Player->CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human ?
			Player->SpectralWeaponComponent->InitializeSpectralWeaponComponent(false) :
			Player->SpectralWeaponComponent->InitializeSpectralWeaponComponent(true);
		
		Destroy();
	}
}