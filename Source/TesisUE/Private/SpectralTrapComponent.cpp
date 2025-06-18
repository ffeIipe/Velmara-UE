#include "SpectralTrapComponent.h"
#include "Components/SphereComponent.h"
#include <Player/PlayerMain.h>
#include <Kismet/GameplayStatics.h>
#include <DamageTypes/SpectralTrapDamageType.h>

USpectralTrapComponent::USpectralTrapComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerVolume"));
	SphereCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Ignore);
}

void USpectralTrapComponent::BeginPlay()
{
	Super::BeginPlay();

	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &USpectralTrapComponent::OnSphereBeginOverlap);

	Instigator = Cast<AController>(GetOwner());
}

void USpectralTrapComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APlayerMain* PlayerRef = Cast<APlayerMain>(OtherActor))
	{
		FVector LaunchDirection = (GetOwner()->GetActorLocation() - PlayerRef->GetActorLocation()).GetSafeNormal();
		LaunchDirection *= -1; 
		
		LaunchDirection.X *= LaunchStrenght.X;
		LaunchDirection.Y *= LaunchStrenght.Y;
		LaunchDirection.Z *= LaunchStrenght.Z;

		UGameplayStatics::ApplyDamage(
			PlayerRef,
			Damage,
			Instigator,
			GetOwner(),
			USpectralTrapDamageType::StaticClass()
		);

		if (IHitInterface* PlayerGetHit = Cast<IHitInterface>(PlayerRef))
		{
			PlayerGetHit->Execute_GetHit(PlayerRef, LaunchDirection, USpectralTrapDamageType::StaticClass(), Damage);
		}
	}
}