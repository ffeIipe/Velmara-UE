#include "SceneEvents/Trigger.h"
#include "Components/BoxComponent.h"
#include "Entities/Entity.h"
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
	
	const TScriptInterface<IControllerProvider> ControllerProvider = OtherActor;

	if (!ControllerProvider) return;
	if (ControllerProvider->GetEntityController() == GetWorld()->GetFirstPlayerController())
	{
		HitActor = OtherActor;
		
		if (OnPlayerControllerBeginOverlap.IsBound())
			OnPlayerControllerBeginOverlap.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, "PlayerControllerBeginOverlap");
		}
	}

	if (Cast<APlayerMain>(OtherActor))
	{
		if (OnPlayerBeginOverlap.IsBound())
			OnPlayerBeginOverlap.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, "PlayerBeginOverlap");
		}
	}
		
	if (ControllerProvider)
	{
		if (OnEntityBeginOverlap.IsBound())
			OnEntityBeginOverlap.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Green, "EntityBeginOverlap");
		}
	}
}

void ATrigger::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const UWorld* World = GetWorld(); !IsValid(World)) return;
	
	if (!OtherActor) return;
	const TScriptInterface<IControllerProvider> ControllerProvider = Cast<APawn>(OtherActor);

	if (!ControllerProvider) return;
	if (ControllerProvider->GetEntityController() == GetWorld()->GetFirstPlayerController())
	{
		if (OnPlayerControllerEndOverlap.IsBound())
		{
			OnPlayerControllerEndOverlap.Broadcast();
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "PlayerControllerEndOverlap");
		}
	}

	if (Cast<APlayerMain>(OtherActor))
	{
		if (OnPlayerEndOverlap.IsBound())
			OnPlayerEndOverlap.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "PlayerEndOverlap");
		}
	}
	
	if (ControllerProvider)
	{
		if (OnEntityEndOverlap.IsBound())
			OnEntityEndOverlap.Broadcast();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "EntityEndOverlap");
		}
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