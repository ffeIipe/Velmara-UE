#include "SceneEvents/Trigger.h"
#include "Components/BoxComponent.h"
#include "Components/PossessionComponent.h"
#include "Player/PlayerMain.h"
#include "Enemy/Enemy.h"
#include <Kismet/GameplayStatics.h>

ATrigger::ATrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	RootComponent = BoxCollider;
}

void ATrigger::BeginPlay()
{
	Super::BeginPlay();

	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &ATrigger::OnSphereBeginOverlap);
	BoxCollider->OnComponentEndOverlap.AddDynamic(this, &ATrigger::OnSphereEndOverlap);
}

void ATrigger::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APlayerMain* PlayerTemp = Cast<APlayerMain>(OtherActor))
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Player Begin Overlap"));*/

		Player = PlayerTemp;

		if (OnPlayerBeginOverlap.IsBound())
		{
			OnPlayerBeginOverlap.Broadcast();
		}
	}
	else if (AEnemy* EnemyTemp = Cast<AEnemy>(OtherActor))
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::White, FString("Enemy Begin Overlap"));*/

		if (EnemyTemp->GetPossessionComponent()->GetPossessedEntity() != nullptr)
		{
			/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, FString("Enemy has a Possession Owner valid..."));*/

			if (OnPlayerBeginOverlap.IsBound())
			{
				OnPlayerBeginOverlap.Broadcast();
			}
		}
		/*else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Emerald, FString("Invalid Possession Owner..."));*/
	}
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    APlayerMain* LeavingPlayer = Cast<APlayerMain>(OtherActor);

    if (IsValid(Player) && Player == LeavingPlayer)
    {
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player End Overlap"));*/

		if (OnPlayerEndOverlap.IsBound())
		{
			OnPlayerEndOverlap.Broadcast();
		}

        Player->SetOverlappingItem(nullptr);
        Player = nullptr;
    }
    else if (LeavingPlayer)
    {
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player End Overlap"));*/

		if (OnPlayerEndOverlap.IsBound())
		{
			OnPlayerEndOverlap.Broadcast();
		}

        LeavingPlayer->SetOverlappingItem(nullptr);
		Player = nullptr;
    }
}

void ATrigger::DisableCollision()
{
	if (BoxCollider)
	{
		BoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	SetActorEnableCollision(false);
}