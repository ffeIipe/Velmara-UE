#include "Components/SpectralWeaponComponent.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"

#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

#include "Interfaces/HitInterface.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"

#include "DamageTypes/SpectralTrapDamageType.h"


USpectralWeaponComponent::USpectralWeaponComponent()
{
    SpectralWeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpectralWeaponMesh"));
    GetSpectralWeaponMeshComponent()->SetVisibility(false);

    MaxAmmo = 3;
    CurrentAmmo = MaxAmmo;
}

void USpectralWeaponComponent::InitializeSpectralWeaponComponent(bool bShouldBeVisible)
{
    bWasInitialized = true;

    if (GetSpectralWeaponMeshComponent())
    {
        GetSpectralWeaponMeshComponent()->SetVisibility(bShouldBeVisible);
    }
}

void USpectralWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerInstigator = GetOwner()->GetInstigator();
    OwnerCharacter = Cast<ACharacter>(GetOwner());
    OwnerController = Cast<APlayerController>(OwnerInstigator->GetController());
    OwnerAttributeComponent = GetOwner()->GetComponentByClass<UAttributeComponent>();

    !bWasInitialized ? EnableSpectralWeapon(false) : EnableSpectralWeapon(true);

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
    if (!bWasInitialized) return;

    if (OwnerAttributeComponent->RequiresEnergy(PrimaryEnergyCost))
    {
        if (CurrentAmmo >= 1 && !bIsReloading && bIsFireEnable)
        {
            Fire(true);
            CurrentAmmo--;
            SetTimer(TimerHandle_BetweenPrimaryShots, .2f, &USpectralWeaponComponent::EnableFire);
        }
        else Reload();
        //TODO: else out of ammo sound  
    }
    else
    {
        //TODO: out of energy
    }
}

void USpectralWeaponComponent::SecondaryFire()
{
    if (!bWasInitialized) return;

    if (OwnerAttributeComponent->RequiresEnergy(SecondaryEnergyCost))
    {
        if (CurrentAmmo == 3 && !bIsReloading && bIsFireEnable)
        {
            Fire(false);
            CurrentAmmo = 0;
            SetTimer(TimerHandle_BetweenPrimaryShots, 1.f, &USpectralWeaponComponent::EnableFire);
        }
        else Reload();
        //TODO: else out of ammo sound   
    }
    else
    {
        //TODO: out of energy
    }
}

void USpectralWeaponComponent::Fire(bool bIsPrimary)
{
    bIsFireEnable = false;
   
    if (!OwnerInstigator || !OwnerCharacter|| !OwnerController) return;

    FVector TraceStart;
    FRotator CameraRotation;
    OwnerController->GetPlayerViewPoint(TraceStart, CameraRotation);

    FVector TraceDirection = CameraRotation.Vector();
    FVector TraceEnd = TraceStart + (TraceDirection * 10000.f);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.AddIgnoredActor(OwnerInstigator);
    QueryParams.bTraceComplex = true;
    QueryParams.bReturnPhysicalMaterial = false;

    if (MuzzleFlash)
    {
        UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, GetSpectralWeaponMeshComponent(), FName("MuzzleSocket"));
    }

    OwnerCharacter->PlayAnimMontage(SpectralFireAnimation);
    UGameplayStatics::PlayWorldCameraShake(this, CameraShake, GetOwner()->GetActorLocation(), 0.f, 500.f);
    OwnerAttributeComponent->DecreaseEnergyBy(bIsPrimary ? PrimaryEnergyCost : SecondaryEnergyCost);
    
    int32 NumTraces = bIsPrimary ? 1 : Shells;
    float CurrentSpreadAngle = bIsPrimary ? 0.f : SpreadAngle;

    for (int32 i = 0; i < NumTraces; ++i)
    {
        if (FireSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetOwner()->GetActorLocation());
        }

        FVector CurrentTraceEnd = TraceEnd;
        if (!bIsPrimary)
        {
            FVector SpreadDirection = FMath::VRandCone(TraceDirection, FMath::DegreesToRadians(CurrentSpreadAngle));
            CurrentTraceEnd = TraceStart + (SpreadDirection * 10000.f);
        }

        FHitResult Hit;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            Hit,
            TraceStart,
            CurrentTraceEnd,
            ECC_Visibility,
            QueryParams
        );

        if (bHit)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                float BaseDamage = bIsPrimary ? PrimaryDamage : SecondaryDamage;
                UGameplayStatics::ApplyPointDamage(
                    Hit.GetActor(),
                    BaseDamage,
                    TraceDirection,
                    Hit,
                    OwnerController,             //who cause the dmg?
                    GetOwner(),                  //what cause the dmg?
                    USpectralTrapDamageType::StaticClass()   //type of dmg
                );

                if (IHitInterface* Entity = Cast<IHitInterface>(HitActor))
                {
                    Entity->Execute_GetHit(Hit.GetActor(), Hit.ImpactPoint, USpectralTrapDamageType::StaticClass());
                    //decals in Hit.ImpactPoint
                }
                else
                {
                    DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 12, FColor::Green, false, 2.0f);
                }
            }
        }
        else return;

        DrawDebugLine(GetWorld(), TraceStart, bHit ? Hit.ImpactPoint : CurrentTraceEnd, FColor::Red, false, 2.0f, 0, 1.0f);
    }
}

void USpectralWeaponComponent::Reload()
{
    if (bIsReloading || CurrentAmmo == MaxAmmo) return;
    
    bIsReloading = true;

    if (!OwnerCharacter) return;
    OwnerCharacter->PlayAnimMontage(SpectralReloadAnimation);
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
    if (!bWasInitialized) return;

    GetSpectralWeaponMeshComponent()->SetupAttachment(InParent, SocketName);
}

void USpectralWeaponComponent::EnableSpectralWeapon(bool Enable)
{
    if (GetSpectralWeaponMeshComponent() && bWasInitialized)
    {
        GetSpectralWeaponMeshComponent()->SetVisibility(Enable);
    }
    else
    {
        GetSpectralWeaponMeshComponent()->SetVisibility(false);
    }
}