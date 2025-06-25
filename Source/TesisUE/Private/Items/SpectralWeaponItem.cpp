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
    if (PlayerReference->CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
    {
        PlayerReference->PlayAnimMontage(EquipMontage, 1.f, FName("Equip"));
    }
    else
    {
        PlayerReference->PlayAnimMontage(EquipMontage, 1.f, FName("Unequip"));
    }
    
    if (PlayerReference && PlayerReference->SpectralWeaponComponent)
    {
        PlayerReference->SpectralWeaponComponent->InitializeSpectralWeaponComponent();

        Destroy(); 
    }
}