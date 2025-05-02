#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/AttributeComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DamageTypes/SpectralTrapDamageType.h"

ASpectralTrap::ASpectralTrap()
{
	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));
	AttributeComponent->AttachShield(GetRootComponent(), FName(""));
}

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		OverlappingPlayer = Player;
		ApplyTrapDamage();
	}
}

void ASpectralTrap::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	if (Player && Player == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
	}
}

void ASpectralTrap::ApplyTrapDamage()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, FString("ASpectralTrap::ApplyTrapDamage"));
	AController* InstigatorController = OverlappingPlayer ? OverlappingPlayer->GetController() : nullptr;

	UGameplayStatics::ApplyDamage(
		OverlappingPlayer,
		Damage,
		InstigatorController,
		this,
		USpectralTrapDamageType::StaticClass()
	);
}

float ASpectralTrap::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AttributeComponent->GetShieldMesh())
	{
		if (AttributeComponent->IsShielded())
		{
			AttributeComponent->ReceiveShieldDamage(DamageAmount);
		}
		else if (!AttributeComponent->bIsDisarmed)
		{
			AttributeComponent->DettachShield();
			BoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetLifeSpan(5.f);
		}
	}

	return DamageAmount;
}