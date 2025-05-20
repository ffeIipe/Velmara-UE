#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DamageTypes/SpectralTrapDamageType.h"

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		OverlappingPlayer = Player;
		ApplyTrapDamage(SweepResult.ImpactPoint);
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

void ASpectralTrap::ApplyTrapDamage(FVector ImpactPoint)
{
	UGameplayStatics::ApplyDamage(
		OverlappingPlayer,
		Damage,
		OverlappingPlayer->GetController(),
		this,
		USpectralTrapDamageType::StaticClass()
	);

	if (OverlappingPlayer)
	{
		if (IHitInterface* Entity = Cast<IHitInterface>(OverlappingPlayer))
		{
			Entity->Execute_GetHit(OverlappingPlayer, ImpactPoint, USpectralTrapDamageType::StaticClass());
;		}
	}
}