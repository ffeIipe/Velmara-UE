#include "Items/SpectralWeaponItem.h"
#include "Player/PlayerMain.h"
#include "Components/SpectralWeaponComponent.h"
#include "Components/CharacterStateComponent.h"
#include <SceneEvents/NewGameStateBase.h>
#include <Kismet/GameplayStatics.h>

void ASpectralWeaponItem::Use(ACharacter* Character)
{
    Super::Use(Character);

    APlayerMain* PlayerReference = Cast<APlayerMain>(Character);

    if (PlayerReference && PlayerReference->SpectralWeaponComponent)
    {
        PlayerReference->SpectralWeaponComponent->InitializeSpectralWeaponComponent();

        Destroy(); 
    }
}