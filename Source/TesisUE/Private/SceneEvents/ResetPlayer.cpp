#include "SceneEvents/ResetPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerMain.h"
#include "GameFramework/PlayerStart.h"
#include "Entities/Entity.h"
#include <SceneEvents/NewGameInstance.h>

void AResetPlayer::BeginPlay()
{
    Super::BeginPlay();

    PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())); 
}

void AResetPlayer::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AEntity* OverlappingActor = Cast<AEntity>(OtherActor))
    {
        if (APlayerMain* PlayerRef = Cast<APlayerMain>(OverlappingActor))
        {
            if (PlayerRef->GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
            {
                PlayerRef->ToggleForm();
            }
        }

        if (UNewGameInstance* GameInst = GetGameInstance<UNewGameInstance>())
        {
            GameInst->LoadPlayerProgress(GameInst->ActiveSaveSlotIndex);
        }
    }
}