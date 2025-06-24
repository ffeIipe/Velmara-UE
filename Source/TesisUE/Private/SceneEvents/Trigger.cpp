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
	APlayerMain* PlayerTemp = Cast<APlayerMain>(OtherActor);
	if (PlayerTemp)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Player Begin Overlap"));

		Player = PlayerTemp;

		if (OnPlayerBeginOverlap.IsBound())
		{
			OnPlayerBeginOverlap.Broadcast();
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player End Overlap"));

		if (OnPlayerEndOverlap.IsBound())
		{
			OnPlayerEndOverlap.Broadcast();
		}

        Player->SetOverlappingItem(nullptr);
        Player = nullptr;
    }
    else if (LeavingPlayer)
    {
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Player End Overlap"));

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
}