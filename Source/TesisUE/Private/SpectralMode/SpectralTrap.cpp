#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/AttributeComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DamageTypes/SpectralTrapDamageType.h"

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
	AController* InstigatorController = OverlappingPlayer ? OverlappingPlayer->GetController() : nullptr;

	UGameplayStatics::ApplyDamage(
		OverlappingPlayer,
		Damage,
		InstigatorController,
		this,
		USpectralTrapDamageType::StaticClass()
	);
}