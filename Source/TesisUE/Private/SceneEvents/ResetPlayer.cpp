#include "SceneEvents/ResetPlayer.h"
#include "Entities/Entity.h"
#include "SceneEvents/VelmaraGameInstance.h"

void AResetPlayer::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (const AEntity* OverlappingActor = Cast<AEntity>(OtherActor))
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, "TODO! Fix ResetPlayer.cpp");
        
        if (UVelmaraGameInstance* VelmaraGameInstance = Cast<UVelmaraGameInstance>(GetWorld()->GetGameInstance()))
        {
            VelmaraGameInstance->LoadGame(VelmaraGameInstance->ActiveSaveSlotIndex);
        }
    }
}