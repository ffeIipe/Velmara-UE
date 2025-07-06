// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/EnemySpectralProjectile.h"
#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>
#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"

void AEnemySpectralProjectile::BeginPlay()
{
	Super::BeginPlay();

	ActorsToIgnore.Add(this);
}

void AEnemySpectralProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IHitInterface* HitInterface = Cast<IHitInterface>(OtherActor))
	{
		if (!ActorsToIgnore.Contains(OtherActor))
		{
			UGameplayStatics::ApplyDamage(
				OtherActor,
				Damage,
				GetInstigator()->GetController(),
				this,
				UDamageType::StaticClass()
			);

			FDamageEvent DamageEvent(UDamageType::StaticClass());
			HitInterface->Execute_GetHit(OtherActor, GetOwner(), SweepResult.ImpactPoint, DamageEvent, Damage);

			ActorsToIgnore.AddUnique(OtherActor);
		}
	}	
}
