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
	if (!OtherActor) return;

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	const AController* Controller = Pawn->GetController();
	if (!Controller) return;

	if (Controller == GetWorld()->GetFirstPlayerController())
	{
		HitActor = OtherActor;
		
		if (OnPlayerControllerBeginOverlap.IsBound())
			OnPlayerControllerBeginOverlap.Broadcast();
	}

	if (Cast<APlayerMain>(OtherActor))
	{
		if (OnPlayerBeginOverlap.IsBound())
			OnPlayerBeginOverlap.Broadcast();
	}
		
	if (Controller)
	{
		if (OnEntityBeginOverlap.IsBound())
			OnEntityBeginOverlap.Broadcast();
	}
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const UWorld* World = GetWorld(); !IsValid(World)) return;
	
	if (!OtherActor) return;

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	const AController* Controller = Pawn->GetController();
	if (!Controller) return;

	if (Controller == GetWorld()->GetFirstPlayerController())
	{
		if (OnPlayerControllerEndOverlap.IsBound())
			OnPlayerControllerEndOverlap.Broadcast();
	}

	if (Cast<APlayerMain>(OtherActor))
	{
		if (OnPlayerEndOverlap.IsBound())
			OnPlayerEndOverlap.Broadcast();
	}
	
	if (Controller)
	{
		if (OnEntityEndOverlap.IsBound())
			OnEntityEndOverlap.Broadcast();
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