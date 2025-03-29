// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/EnemySpectralProjectile.h"
#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>

void AEnemySpectralProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemySpectralProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	if (OtherActor == Player && !ActorsToIgnore.Contains(Player))
	{
		UGameplayStatics::ApplyDamage(
			Player,
			Damage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
		);
		ActorsToIgnore.AddUnique(OtherActor);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.f, FColor::Cyan, FString("Actor Overlapped " + OtherActor->GetDebugName(OtherActor)));
	}
}
