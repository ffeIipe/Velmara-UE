#include "SpectralTrapComponent.h"
#include "Components/SphereComponent.h"
#include <Player/PlayerMain.h>
#include <Kismet/GameplayStatics.h>
#include "Engine/DamageEvents.h"
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
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &USpectralTrapComponent::OnSphereEndOverlap);

	Instigator = Cast<AController>(GetOwner());
}

void USpectralTrapComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OverlappingPlayer = Cast<APlayerMain>(OtherActor);

	if (OverlappingPlayer)
	{
		/*GEngine->AddOnScreenDebugMessage(845, 1.f, FColor::Green, FString("Overlapping Player..."));*/

		GetWorld()->GetTimerManager().SetTimer(
			ContinuousDamageTimerHandle,
			this,
			&USpectralTrapComponent::DealContinuousDamage,
			DamageInterval,
			true
		);

		DealContinuousDamage();
	}
}

void USpectralTrapComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OverlappingPlayer = Cast<APlayerMain>(OtherActor);

	if (OverlappingPlayer)
	{
		/*GEngine->AddOnScreenDebugMessage(845, 1.f, FColor::Red, FString("End Overlapping Player..."));*/

		GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);

		FinishDamaging();
		OverlappingPlayer = nullptr;
	}
}

void USpectralTrapComponent::DealContinuousDamage()
{
	if (OverlappingPlayer)
	{
		/*GEngine->AddOnScreenDebugMessage(678, 0.5f, FColor::Purple, FString("Applying continuous damage..."));*/

		UGameplayStatics::ApplyDamage(OverlappingPlayer, Damage, Instigator, GetOwner(), USpectralTrapDamageType::StaticClass());

		if (IHitInterface* PlayerGetHit = Cast<IHitInterface>(OverlappingPlayer))
		{
			FDamageEvent DamageEvent(UDamageType::StaticClass());
			PlayerGetHit->Execute_GetHit(OverlappingPlayer, GetOwner(), FVector::ZeroVector, DamageEvent, Damage);
		}
	}
}

void USpectralTrapComponent::FinishDamaging()
{
	if (OverlappingPlayer)
	{
		OverlappingPlayer->RemoveStunBehavior();
	}
	DestroyComponent();
}