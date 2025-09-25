#include "SceneEvents/ResetPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerMain.h"
#include "GameFramework/PlayerStart.h"
#include "Entities/Entity.h"
#include <SceneEvents/VelmaraGameInstance.h>

#include "Components/CharacterStateComponent.h"

void AResetPlayer::BeginPlay()
{
    Super::BeginPlay();

    PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())); 
}

void AResetPlayer::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AEntity* OverlappingActor = Cast<AEntity>(OtherActor))
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, "TODO! Fix ResetPlayer.cpp");
        
        /*if (APlayerMain* PlayerRef = Cast<APlayerMain>(OverlappingActor))
        {
            if (PlayerRef->GetCharacterStateComponent() .Mode == ECharacterModeStates::ECMS_Spectral)
            {
                PlayerRef->ToggleForm();
            }
        }*/

        if (UVelmaraGameInstance* GameInst = GetGameInstance<UVelmaraGameInstance>())
        {
            GameInst->LoadPlayerProgress(GameInst->ActiveSaveSlotIndex);
        }
    }
}