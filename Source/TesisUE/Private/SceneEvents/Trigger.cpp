#include "SceneEvents/Trigger.h"
#include "Components/BoxComponent.h"
#include "Player/PlayerMain.h"

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
	Player = Cast<APlayerMain>(OtherActor);
	if (GEngine && Player) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("ATrigger::OnSphereBeginOverlap"));
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return;

    APlayerMain* LeavingPlayer = Cast<APlayerMain>(OtherActor);

    if (IsValid(Player) && Player == LeavingPlayer)
    {
        Player->SetOverlappingItem(nullptr);
        Player = nullptr;
    }
    else if (LeavingPlayer)
    {
        LeavingPlayer->SetOverlappingItem(nullptr);
    }
}

void ATrigger::DisableCollision()
{
	if (BoxCollider)
	{
		BoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}