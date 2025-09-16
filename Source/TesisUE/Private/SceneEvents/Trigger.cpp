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
		Player = PlayerTemp;

		if (OnPlayerBeginOverlap.IsBound())
		{
			OnPlayerBeginOverlap.Broadcast();
		}
	}
	else if (AEnemy* EnemyTemp = Cast<AEnemy>(OtherActor))
	{
		if (EnemyTemp->GetPossessionComponent()->GetPossessedEntity() != nullptr)
		{
			if (OnPlayerBeginOverlap.IsBound())
			{
				OnPlayerBeginOverlap.Broadcast();
			}
		}
	}
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    APlayerMain* LeavingPlayer = Cast<APlayerMain>(OtherActor);

    if (IsValid(Player) && Player == LeavingPlayer)
    {
		if (OnPlayerEndOverlap.IsBound())
		{
			OnPlayerEndOverlap.Broadcast();
		}

        //Player->SetOverlappingItem(nullptr);
        Player = nullptr;
    }
    else if (LeavingPlayer)
    {
		if (OnPlayerEndOverlap.IsBound())
		{
			OnPlayerEndOverlap.Broadcast();
		}

        //LeavingPlayer->SetOverlappingItem(nullptr);
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