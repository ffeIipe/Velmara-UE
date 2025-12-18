#include "SceneEvents/LevelEvents/SunDamage.h"

#include "DamageTypes/EnvironmentalDamage.h"
#include "Engine/DamageEvents.h"
#include "Engine/DirectionalLight.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/GameplayStatics.h"


ASunDamage::ASunDamage()
{
	PrimaryActorTick.TickInterval = DamageInterval;
	PrimaryActorTick.bCanEverTick = true;
}

void ASunDamage::BeginPlay()
{
	Super::BeginPlay();

	DirectionalLight = Cast<ADirectionalLight>(UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()));
	PlayerActor = GetWorld()->GetFirstPlayerController()->GetPawn();
	PlayerHittable = PlayerActor;
	
	if (!DirectionalLight || !PlayerActor || !DamageCurve)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Red, "---- Directional Light, Player or Damage Curve not found! -----");
		}
		PrimaryActorTick.bCanEverTick = false;
	}

	PrimaryActorTick.TickInterval = DamageInterval;
	SunLocation = DirectionalLight->GetActorLocation();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 10.f, FColor::Blue, SunLocation.ToString());
	}
}

void ASunDamage::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GEngine && bIsDebugEnabled)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, .1f, FColor::Cyan, GetName() + " is ticking...");
	}

	const TArray<AActor*> IgnoreActors;
	FHitResult Hit;

	//SunLocation = FVector(PlayerActor->GetActorLocation().X, PlayerActor->GetActorLocation().Y, SunLocation.Z);
	
	const bool bIsWallHit = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		SunLocation,
		PlayerActor->GetActorLocation(),
		UEngineTypes::ConvertToTraceType(WallCollisionChannel),
		false,
		IgnoreActors,
		DrawDebugType,
		Hit,
		true
		) &&
			UKismetSystemLibrary::LineTraceSingle(
			GetWorld(),
			SunLocation,
			PlayerActor->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f),
			UEngineTypes::ConvertToTraceType(WallCollisionChannel),
			false,
			IgnoreActors,
			DrawDebugType,
			Hit,
			true
			) &&
				UKismetSystemLibrary::LineTraceSingle(
		 GetWorld(),
		SunLocation,
		PlayerActor->GetActorLocation()+ FVector(0.0f, 0.0f, -80.0f),
		UEngineTypes::ConvertToTraceType(WallCollisionChannel),
		false,
		IgnoreActors,
		DrawDebugType,
		Hit,
		true
		);
	
	if (!bIsWallHit)
	{
		TimeOnTarget += DamageInterval;

		if (DamageCurve && MinDamage < MaxDamage)
		{
			Damage = MaxDamage * DamageCurve->GetFloatValue(TimeOnTarget);
		}
		
		UGameplayStatics::ApplyDamage(
			PlayerActor,
			Damage,
			nullptr,
			this,
			UEnvironmentalDamage::StaticClass()
			);
		
		const FDamageEvent DamageEvent(UEnvironmentalDamage::StaticClass());
		if (PlayerHittable)
		{
			PlayerHittable->GetHit(
				this,
				Hit.ImpactPoint,
				DamageEvent,
				MaxDamage
				);
		}
	}
	else
	{
		TimeOnTarget = 0.f;
		Damage = MinDamage;
	}
}