#include "Items/SpectralWeaponItem.h"
#include "Player/PlayerMain.h"
#include "Components/SpectralWeaponComponent.h"
#include "Components/CharacterStateComponent.h"
#include <SceneEvents/NewGameStateBase.h>
#include <Kismet/GameplayStatics.h>
#include "Tutorial/PromptWidgetComponent.h"

void ASpectralWeaponItem::Use(ACharacter* Character)
{
    Super::Use(Character);

    APlayerMain* PlayerReference = Cast<APlayerMain>(Character);
    if (PlayerReference->CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
    {
        PlayerReference->Equipping(false);
    }
    
    if (PlayerReference && PlayerReference->SpectralWeaponComponent)
    {
        FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
        ItemMesh->AttachToComponent(Character->GetMesh(), TransformRules, FName("RightHandSocketWeapon"));

        PlayerReference->SpectralWeaponComponent->GetSpectralWeaponMeshComponent()->SetStaticMesh(ItemMesh->GetStaticMesh());
        PlayerReference->SpectralWeaponComponent->InitializeSpectralWeaponComponent();
        PromptWidget->EnablePromptWidget(false);

        Destroy(); 
    }
}