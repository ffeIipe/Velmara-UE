#include "Items/SpectralWeaponItem.h"
#include "Player/PlayerMain.h"
#include "Components/SpectralWeaponComponent.h"
#include "Components/CharacterStateComponent.h"

void ASpectralWeaponItem::Use(ACharacter* Character)
{
    Super::Use(Character);

    APlayerMain* PlayerCasted = Cast<APlayerMain>(Character);

    if (PlayerCasted && PlayerCasted->SpectralWeaponComponent)
    {
        PlayerCasted->CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Human ?
            PlayerCasted->SpectralWeaponComponent->InitializeSpectralWeaponComponent(false) :
            PlayerCasted->SpectralWeaponComponent->InitializeSpectralWeaponComponent(true);

        Destroy(); 
    }
}