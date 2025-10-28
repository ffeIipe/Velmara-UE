#include "SceneEvents/Checkpoint.h"
#include "Entities/Entity.h"
#include "Components/PossessionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/VelmaraGameInstance.h"

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
        if (OverlappingEntity->GetController() == UGameplayStatics::GetPlayerController(GetWorld(), 0))
        {
            DisableCollision();
            
            if (OverlappingEntity->GetPossessionComponent()->IsPossessed())
            {
                if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, FString("Saving from " + OverlappingEntity->GetName()));
                OverlappingEntity->GetPossessionComponent()->GetPossessor()->SetActorTransform(OverlappingEntity->GetActorTransform());
            }

            if (UVelmaraGameInstance* GameInst = Cast<UVelmaraGameInstance>(GetGameInstance()))
            {
                if (GameInst->SavePlayerProgress(GameInst->ActiveSaveSlotIndex, OverlappingEntity))
                {
                    if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Emerald, FString("Player progress saved via GameInstance."));
                    UE_LOG(LogTemp, Log, TEXT("ACheckpoint: Player progress saved via GameInstance. Slot: %d"), GameInst->ActiveSaveSlotIndex);
                }
            }
            else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, FString("Failed to save via VelmaraGameInstance. The current GameInstance is: " + GetGameInstance()->GetName()));

            Destroy();
        }
    }
}