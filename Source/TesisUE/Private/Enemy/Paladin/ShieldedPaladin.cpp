// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Paladin/ShieldedPaladin.h"

AShieldedPaladin::AShieldedPaladin()
{
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(GetMesh(), TEXT("LeftHandSocket"));
}

void AShieldedPaladin::Disarm()
{
    if (ShieldMesh)
    {
        ShieldMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        ShieldMesh->SetSimulatePhysics(true);
        ShieldMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        bIsDisarmed = true;
    }
}

float AShieldedPaladin::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDisarmed)
    {
        Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    }
    return DamageAmount;
}

void AShieldedPaladin::BeginPlay()
{
    Super::BeginPlay();
}
