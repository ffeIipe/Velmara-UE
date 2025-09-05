// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapons/Pistol.h"

#include "NiagaraFunctionLibrary.h"
#include "DamageTypes/PistolDamage.h"
#include "Engine/DamageEvents.h"
#include "Entities/Entity.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/EffectsManager.h"

APistol::APistol()
{
	OnFire.AddDynamic(this, &APistol::PlayEffects);
}

void APistol::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MaxAmmo;
}

void APistol::UsePrimaryAttack()
{
	Super::UsePrimaryAttack();
	PrimaryShoot();
}

void APistol::UseSecondaryAttack()
{
	Super::UseSecondaryAttack();
	SecondaryShoot();
}

void APistol::PrimaryShoot()
{
	Fire();
}

void APistol::SecondaryShoot()
{
}

void APistol::PlayEffects()
{
	const FVector SocketLocation = ItemMesh->GetSocketLocation(FName("MuzzleFlashSocket"));
	const FRotator SocketRotation = ItemMesh->GetSocketRotation(FName("MuzzleFlashSocket"));

	if (MuzzleFlash)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			MuzzleFlash,
			SocketLocation,
			SocketRotation,
			FVector(1)
		);
	}

	if (ShootSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ShootSound,
			SocketLocation,
			SocketRotation,
			1.f
		);
	}

	if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
	{
		EffectsManager->CameraShake(ECameraShakePreset::ECSP_ShotHit, SocketLocation);
	}
}

void APistol::Fire()
{
	if (!AttributeProvider->RequiresEnergy(EnergyToDecrease)) return;

	if (CurrentAmmo >= 1 && !bIsReloading && bIsFireEnabled)
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Shooting");*/
		bIsFireEnabled = false;
		CurrentAmmo--;
		SetTimer(TimerHandle_BetweenShots, FireEnableTime, &APistol::EnableFire);

		if (OnFire.IsBound()) OnFire.Broadcast();

		AttributeProvider->IncreaseEnergy(-EnergyToDecrease);
		AnimatorProvider->PlayAnimMontage(PrimaryFireMontage);
		
		FVector TraceStart;
		FRotator CameraRotation;
		ControllerProvider->GetEntityController()->GetPlayerViewPoint(TraceStart, CameraRotation);

		const FVector TraceDirection = CameraRotation.Vector();
		const FVector TraceEnd = TraceStart + TraceDirection * 1000000.f;

		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());
		CollisionParams.bTraceComplex = true;
		CollisionParams.bTraceComplex = false;
		
		FHitResult HitResult;

		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			CollisionParams
		);
		
		if (!bHit) return;
		
		if (const TScriptInterface<IHitInterface> HitInterface = HitResult.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(
			HitResult.GetActor(),
			Damage,
			TraceDirection,
			HitResult,
			ControllerProvider->GetEntityController(),
			GetOwner(),
			UPistolDamage::StaticClass()
			);

			const FDamageEvent DamageEvent(UDamageType::StaticClass());
			HitInterface->GetHit(GetOwner(), HitResult.ImpactPoint, DamageEvent, Damage);
		}
	}
	else if (CurrentAmmo <= 0)
		Reload();
}

void APistol::EnableFire()
{
	bIsFireEnabled = true;
}

void APistol::Reload()
{
	if (bIsReloading || CurrentAmmo == MaxAmmo) return;

	/*if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Reloading");*/
	bIsReloading = true;
	
	AnimatorProvider->PlayAnimMontage(ReloadMontage);
	SetTimer(TimerHandle_Reload, 1.f, &APistol::FinishReload);
}

void APistol::FinishReload()
{
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;
}

void APistol::SetTimer(FTimerHandle TimerHandle, const float Time, void (APistol::*InTimerMethod)())
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(TimerHandle);
	TimerManager.SetTimer(TimerHandle, this, InTimerMethod, Time, false);
}
