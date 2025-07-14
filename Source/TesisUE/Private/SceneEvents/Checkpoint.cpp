#include "SceneEvents/Checkpoint.h"
#include "Entities/Entity.h"
#include "Components/MementoComponent.h"
#include "Components/PossessionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameStateBase.h"
#include "SceneEvents/NewGameInstance.h"

void ACheckpoint::OnSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (AEntity* OverlappingEntity = Cast<AEntity>(OtherActor))
    {
        DisableCollision();

        if (OverlappingEntity->GetPossessionComponent()->IsPossessed())
        {
            OverlappingEntity->GetPossessionComponent()->GetPossessingEntity()->SetActorTransform(OverlappingEntity->GetActorTransform());
        }

        UNewGameInstance* GameInst = Cast<UNewGameInstance>(GetGameInstance());
        if (GameInst)
        {
            if (GameInst->SavePlayerProgress(GameInst->ActiveSaveSlotIndex, OverlappingEntity))
            {
                if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Emerald, FString("Player progress saved via GameInstance."));
                UE_LOG(LogTemp, Log, TEXT("ACheckpoint: Player progress saved via GameInstance. Slot: %d"), GameInst->ActiveSaveSlotIndex);
            }
        }

        Destroy();
    }
}