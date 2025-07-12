// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/ResetPlayer.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "Player/PlayerMain.h"
#include "GameFramework/PlayerStart.h"
#include "Components/MementoComponent.h"
#include "Entities/Entity.h"

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

       /* APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
        if (Cast<APlayerController>(OverlappingActor->GetController()) == PlayerController)
        */{
            OverlappingActor->SetActorTransform(OverlappingActor->GetMementoComponent()->GetLastSavedTransform());
        }
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("ResetPlayer: Overlapping actor!"));
    }
    else if (GEngine) 
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("ResetPlayer: Overlapping actor is not an Entity!"));
	}
}