// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/SpectralProjectile.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/TimelineComponent.h"
#include "Player/PlayerMain.h"

ASpectralProjectile::ASpectralProjectile()
{
    
}

void ASpectralProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (Player)
    {
        Target = Player->GetEnemyTarget();
    }
}


