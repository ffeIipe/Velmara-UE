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
	SphereCollider->OnComponentEndOverlap.AddDynamic(this, &USpectralTrapComponent::OnSphereEndOverlap);

	Instigator = Cast<AController>(GetOwner());
}

void USpectralTrapComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Player = Cast<APlayerMain>(OtherActor);

	if (Player)
	{
		GEngine->AddOnScreenDebugMessage(845, 1.f, FColor::Green, FString("Overlapping Player..."));

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
	Player = Cast<APlayerMain>(OtherActor);

	if (Player)
	{
		GEngine->AddOnScreenDebugMessage(845, 1.f, FColor::Red, FString("End Overlapping Player..."));

		GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);

		Player->RemoveStunBehavior();
		Player = nullptr;
	}
}

void USpectralTrapComponent::ApplySpectralDamage(AActor* Actor, float DamageAmount, AController* InstigatorOf, AActor* DamageCauserOf, TSubclassOf<UDamageType> DamageType)
{
	GEngine->AddOnScreenDebugMessage(678, 3.f, FColor::Purple, FString("USpectralTrapComponent::ApplySpectralDamage"));

	UGameplayStatics::ApplyDamage(
		Actor,
		DamageAmount,
		InstigatorOf,
		DamageCauserOf,
		DamageType
	);
}

void USpectralTrapComponent::DealContinuousDamage()
{
	if (Player)
	{
		GEngine->AddOnScreenDebugMessage(678, 0.5f, FColor::Purple, FString("Applying continuous damage..."));

		ApplySpectralDamage(Player, Damage, Instigator, GetOwner(), USpectralTrapDamageType::StaticClass());

		if (IHitInterface* PlayerGetHit = Cast<IHitInterface>(Player))
		{
			PlayerGetHit->Execute_GetHit(Player, FVector::ZeroVector, USpectralTrapDamageType::StaticClass(), Damage);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);
	}
}
