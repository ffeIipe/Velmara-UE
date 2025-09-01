// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapons/Pistol.h"

#include "Components/CharacterStateComponent.h"
#include "Engine/DamageEvents.h"
#include "Entities/Entity.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CharacterHumanStates.h"

void APistol::BeginPlay()
{
	Super::BeginPlay();
	
}

void APistol::AttachMeshToSocket(USceneComponent* InParent, const FName InSocketName) const
{
	const FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

void APistol::Equip(USceneComponent* InParent, const FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	Super::Equip(InParent, InSocketName, NewOwner, NewInstigator);

	AttachMeshToSocket(InParent, InSocketName);
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	ItemState = EItemState::EIS_Equipped;

	// EnableSwordState(true);

	ControllerProvider = NewOwner;
	OwnerController = ControllerProvider->GetEntityController();
	
	CharacterStateProvider = NewOwner;
	
	if (CharacterStateProvider)
	{
		CharacterStateProvider->SetHumanState(ECharacterHumanStates::ECHS_EquippedSword);
	}
}

void APistol::Fire()
{
	if (CurrentAmmo >= 1 && !bIsReloading && bIsFireEnabled)
	{
		bIsFireEnabled = false;
		CurrentAmmo--;
		SetTimer(TimerHandle_BetweenShots, .65f, &APistol::EnableFire);
		
		FVector TraceStart;
		FRotator CameraRotation;
		OwnerController->GetPlayerViewPoint(TraceStart, CameraRotation);

		const FVector TraceDirection = CameraRotation.Vector();
		const FVector TraceEnd = TraceStart + TraceDirection * 100000.f;

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

		if (bHit)
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				UGameplayStatics::ApplyPointDamage(
					HitActor,
					Damage,
					TraceDirection,
					HitResult,
					OwnerController,
					GetOwner(),
					UDamageType::StaticClass()
					);

				if (IHitInterface* Entity = Cast<IHitInterface>(HitActor))
				{
					const FDamageEvent DamageEvent(UDamageType::StaticClass());
					Entity->GetHit(GetOwner(), HitResult.ImpactPoint, DamageEvent, Damage);
				}
			}
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

	bIsReloading = true;
	if (ACharacter* CharacterOwner = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		CharacterOwner->PlayAnimMontage(ReloadMontage);
		SetTimer(TimerHandle_Reload, 1.f, &APistol::FinishReload);
	}
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
