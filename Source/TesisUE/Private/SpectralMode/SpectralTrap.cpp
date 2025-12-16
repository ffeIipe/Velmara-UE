#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DamageTypes/SpectralTrapDamageType.h"

void ASpectralTrap::BeginPlay()
{
	Super::BeginPlay();

	if (TrapSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), TrapSFX, BoxCollider->GetComponentLocation());
	}
}

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ContinuousDamageTimerHandle,
			this,
			&ASpectralTrap::DealContinuousDamage,
			DamageInterval,
			true
		);

		DealContinuousDamage();

		//ApplyTrapDamage(SweepResult.ImpactPoint);
	}
}

void ASpectralTrap::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	if (Player == OtherActor)
	{
		Player->RemoveStunBehavior();
		Player = nullptr;
		
		GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);
	}
}

void ASpectralTrap::ApplyTrapDamage(FVector ImpactPoint)
{
	UGameplayStatics::ApplyDamage(
		Player,
		Damage,
		Player->GetController(),
		this,
		USpectralTrapDamageType::StaticClass()
	);

	if (Player)
	{
		if (IHitInterface* Entity = Cast<IHitInterface>(Player))
		{
			Entity->Execute_GetHit(Player, GetOwner(), ImpactPoint, USpectralTrapDamageType::StaticClass(), Damage);
;		}
	}
}

void ASpectralTrap::DealContinuousDamage()
{
	if (Player)
	{
		/*GEngine->AddOnScreenDebugMessage(678, 0.5f, FColor::Purple, FString("Applying continuous damage..."));*/

		UGameplayStatics::ApplyDamage(Player, Damage, nullptr, this, USpectralTrapDamageType::StaticClass());

		if (IHitInterface* PlayerGetHit = Cast<IHitInterface>(Player))
		{
			PlayerGetHit->Execute_GetHit(Player, GetOwner(), FVector::ZeroVector, USpectralTrapDamageType::StaticClass(), Damage);
		}
	}
}
