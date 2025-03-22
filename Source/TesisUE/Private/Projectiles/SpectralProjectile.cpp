// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/SpectralProjectile.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/TimelineComponent.h"
#include <Enemy/Enemy.h>
#include "GameFramework/ProjectileMovementComponent.h"

ASpectralProjectile::ASpectralProjectile()
{
	ProjectileTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("ProjectileTimeline"));

	if (Target)
	{

		if (IsHoming)
		{
			ProjectileMovementComponent->bIsHomingProjectile = true;
			ProjectileMovementComponent->HomingAccelerationMagnitude = 2000.f;
			ProjectileMovementComponent->HomingTargetComponent = Target->GetRootComponent();
		}
	}
}

void ASpectralProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("UpdateCurve"));
		ProjectileTimeline->AddInterpFloat(ProjectileCurve, ProgressFunction);
	}

	TrackTarget();
}

void ASpectralProjectile::StartCurve()
{
	if (ProjectileTimeline)
	{
		ProjectileTimeline->PlayFromStart();
	}
}

void ASpectralProjectile::UpdateCurve(float Alpha)
{
	FVector CurrentLocation = GetActorLocation();
	if (Target)
	{
		FVector TargetLocation = FMath::Lerp(CurrentLocation, Target->GetActorLocation(), Alpha);
		SetActorLocation(TargetLocation, true);
	}
}

void ASpectralProjectile::TrackTarget()
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		GetActorLocation(),
		GetActorLocation() + (GetActorForwardVector() * TrackTargetDistance),
		TrackTargetRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		ResultHit,
		true
	);

	AActor* HitActor = ResultHit.GetActor();
	if (HitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trace hit: %s"), *HitActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Trace didn't hit anything"));
	}
}