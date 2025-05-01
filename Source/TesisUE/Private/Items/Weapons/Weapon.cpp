#include "Items/Weapons/Weapon.h"
#include "Items/Item.h"
#include "Player/PlayerMain.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Math/UnrealMathUtility.h"

AWeapon::AWeapon()
{    
    MaxAmmo = 3;
    CurrentAmmo = MaxAmmo;
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    CurrentAmmo = MaxAmmo;
}

void AWeapon::PrimaryFire()
{
    if (CurrentAmmo >= 1)
    {
        CurrentAmmo--;
        UE_LOG(LogTemp, Warning, TEXT("Primary Fire! Ammo left: %d"), CurrentAmmo);
        Fire(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No Ammo for Primary Fire!"));
    }
}

void AWeapon::SecondaryFire()
{
    if (CurrentAmmo == 3)
    {
        CurrentAmmo = 0;
        UE_LOG(LogTemp, Warning, TEXT("Secondary Fire! Ammo left: %d"), CurrentAmmo);
        Fire(false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough Ammo for Secondary Fire! Need 3, have %d"), CurrentAmmo);
        //out of ammo sound
    }
}

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
    Super::Equip(InParent, FName("RightHandSocketWeapon"), NewOwner, NewInstigator);

    SetOwner(NewOwner);
    SetInstigator(NewInstigator);

    if (InParent && ItemMesh)
    {
        FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
        ItemMesh->AttachToComponent(InParent, AttachmentRules, InSocketName);
    }

    if (BoxCollider)
    {
        BoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BoxCollider->SetGenerateOverlapEvents(false);
    }

    Reload();
}

void AWeapon::Unequip()
{
    if (ItemMesh)
    {
        FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepWorld, true);
        ItemMesh->DetachFromComponent(DetachmentRules);
    }

    Super::Unequip(); 

    if (BoxCollider)
    {
        BoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        BoxCollider->SetGenerateOverlapEvents(true);
    }

    SetOwner(nullptr);
    SetInstigator(nullptr);
}

void AWeapon::Fire(bool bIsPrimary)
{
    APawn* MyInstigator = GetInstigator();
    if (!MyInstigator) return;

    APlayerController* PlayerController = Cast<APlayerController>(MyInstigator->GetController());
    if (!PlayerController) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector TraceStart = CameraLocation;

    FVector TraceDirection = CameraRotation.Vector();
    float TraceLength = 10000.f;
    FVector TraceEnd = TraceStart + (TraceDirection * TraceLength);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(MyInstigator);
    QueryParams.bTraceComplex = true;
    QueryParams.bReturnPhysicalMaterial = false;

    if (MuzzleFlash)
    {
        UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, ItemMesh, FName("MuzzleSocket"));
    }
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }

    // --- Lógica de Disparo ---
    int32 NumTraces = bIsPrimary ? 1 : 5; // 1 trazo para revólver, 5 para escopeta (ajustable)
    float SpreadAngle = bIsPrimary ? 0.f : 5.0f; // Ángulo de dispersión en grados para escopeta

    for (int32 i = 0; i < NumTraces; ++i)
    {
        FVector CurrentTraceEnd = TraceEnd;
        if (!bIsPrimary)
        {
            FVector SpreadDirection = FMath::VRandCone(TraceDirection, FMath::DegreesToRadians(SpreadAngle));
            CurrentTraceEnd = TraceStart + (SpreadDirection * TraceLength);
        }

        FHitResult Hit;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            Hit,
            TraceStart,
            CurrentTraceEnd,
            ECC_GameTraceChannel3,
            QueryParams
        );

        DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : CurrentTraceEnd, FColor::Red, false, 2.0f, 0, 1.0f);

        if (bHit)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), *HitActor->GetName());
                float BaseDamage = bIsPrimary ? 50.f : 15.f;
                UGameplayStatics::ApplyPointDamage(
                    HitActor,
                    BaseDamage,
                    TraceDirection,
                    Hit,
                    MyInstigator->GetController(), // Controlador que causó el daño
                    this,           // Actor que causó el daño (el arma)
                    UDamageType::StaticClass() // Tipo de daño (puedes crear subclases)
                );

                //decals in Hit.ImpactPoint
                DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 12, FColor::Green, false, 2.0f);
            }
        }
    }
}

void AWeapon::Reload()
{
    CurrentAmmo = MaxAmmo;
    UE_LOG(LogTemp, Warning, TEXT("Weapon Reloaded. Ammo: %d"), CurrentAmmo);
    //updateUI visuals
}
