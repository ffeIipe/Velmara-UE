// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/PlayerMain.h"
#include "Enemy/Enemy.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(GetRootComponent());
	

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(BoxCollider);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	bEnableDestroyOnCollision = true;

	SetLifeSpan(ProjectileLifetime);

	Player = Cast<APlayerMain>(GetWorld()->GetFirstPlayerController()->GetPawn());

	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnBoxOverlap);
}

void AProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AEnemy>(OtherActor))
	{
		OnProjectileImpact(OtherActor, SweepResult);
	}
	else return;
}

void AProjectile::OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit)
{
}
