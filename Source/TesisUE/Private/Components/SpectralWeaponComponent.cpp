#include "Components/SpectralWeaponComponent.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

#include "Interfaces/HitInterface.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"


USpectralWeaponComponent::USpectralWeaponComponent()
{
    SpectralWeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpectralWeaponMesh"));

    MaxAmmo = 3;
    CurrentAmmo = MaxAmmo;
}

void USpectralWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    if (SpectralWeaponMesh && SpectralWeaponMeshComponent)
    {
        SpectralWeaponMeshComponent->SetStaticMesh(SpectralWeaponMesh);
    }

    CurrentAmmo = MaxAmmo;
}

void USpectralWeaponComponent::EnableFire()
{
    bIsFireEnable = true;
}

void USpectralWeaponComponent::SetTimer(FTimerHandle TimerHandle, float Time, void (USpectralWeaponComponent::*InTimerMethod)())
{
    FTimerManager& TimerManager = GetWorld()->GetTimerManager();
    TimerManager.ClearTimer(TimerHandle);
    TimerManager.SetTimer(TimerHandle, this, InTimerMethod, Time, false);
}

void USpectralWeaponComponent::PrimaryFire()
{
    if (CurrentAmmo >= 1 && !bIsReloading && bIsFireEnable)
    {
        Fire(true);
        CurrentAmmo--;
        SetTimer(TimerHandle_BetweenPrimaryShots, .2f, &USpectralWeaponComponent::EnableFire);
    }
    else Reload();
}

void USpectralWeaponComponent::SecondaryFire()
{
    if (CurrentAmmo == 3 && !bIsReloading && bIsFireEnable)
    {
        Fire(false);
        CurrentAmmo = 0;
        SetTimer(TimerHandle_BetweenPrimaryShots, 1.f, &USpectralWeaponComponent::EnableFire);
    }
    else Reload();
    //else out of ammo sound   
}

void USpectralWeaponComponent::Fire(bool bIsPrimary)
{
    bIsFireEnable = false;

    APawn* MyInstigator = GetOwner()->GetInstigator();
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
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.AddIgnoredActor(MyInstigator);
    QueryParams.bTraceComplex = true;
    QueryParams.bReturnPhysicalMaterial = false;

    if (MuzzleFlash)
    {
        UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, SpectralWeaponMeshComponent, FName("MuzzleSocket"));
    }
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetOwner()->GetActorLocation());
    }

    int32 NumTraces = bIsPrimary ? 1 : 5;
    float SpreadAngle = bIsPrimary ? 0.f : 5.0f;

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
            IHitInterface* Entity = Cast<IHitInterface>(Hit.GetActor());

            if (Entity)
            {
                float BaseDamage = bIsPrimary ? 50.f : 15.f;
                UGameplayStatics::ApplyPointDamage(
                    Hit.GetActor(),
                    BaseDamage,
                    TraceDirection,
                    Hit,
                    MyInstigator->GetController(),  //who cause the dmg?
                    GetOwner(),                     //what cause the dmg?
                    UDamageType::StaticClass()      //type of dmg
                );

                Entity->Execute_GetHit(Hit.GetActor(), Hit.ImpactPoint);

                //decals in Hit.ImpactPoint
                DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 12, FColor::Green, false, 2.0f);
            }
        }
    }
}

void USpectralWeaponComponent::Reload()
{
    if (bIsReloading || CurrentAmmo == MaxAmmo) return;
    
    bIsReloading = true;

    SetTimer(TimerHandle_Reload, ReloadTime, &USpectralWeaponComponent::FinishReload);
}

void USpectralWeaponComponent::FinishReload()
{
    CurrentAmmo = MaxAmmo;
    bIsReloading = false;
    //updateUI visuals
}

void USpectralWeaponComponent::AttachToOwner(USceneComponent* InParent, FName SocketName)
{
    SpectralWeaponMeshComponent->SetupAttachment(InParent, SocketName);
}