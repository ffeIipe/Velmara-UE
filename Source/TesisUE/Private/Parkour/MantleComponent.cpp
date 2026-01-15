#include "Parkour/MantleComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UMantleComponent::UMantleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMantleComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<ACharacter>(GetOwner());
}

bool UMantleComponent::DetectLedge(FLedgeDetectionResult& OutResult) const
{
    OutResult = FLedgeDetectionResult();
    if (!OwnerCharacter) return false;

    const FVector Start = OwnerCharacter->GetActorLocation();
    const FVector Forward = OwnerCharacter->GetActorForwardVector();
    const FVector End = Start + (Forward * ClimbingTraceDistance);

    const EDrawDebugTrace::Type DebugType = bDrawDebugTraces ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

    FHitResult WallHit;
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerCharacter);

    const bool bHitWall = UKismetSystemLibrary::SphereTraceSingle(
        this, Start, End, 20.0f,
        UEngineTypes::ConvertToTraceType(ECC_WorldStatic),
        false, ActorsToIgnore, EDrawDebugTrace::ForDuration, WallHit, true
    );

    if (!bHitWall || !WallHit.bBlockingHit) return false;

    const FVector LedgeStart = WallHit.ImpactPoint + (FVector::UpVector * 150.0f) + (Forward * 10.0f);
    const FVector LedgeEnd = LedgeStart - (FVector::UpVector * 160.0f);

    FHitResult LedgeHit;
    const bool bHitLedge = UKismetSystemLibrary::SphereTraceSingle(
        this, LedgeStart, LedgeEnd, 20.0f,
        UEngineTypes::ConvertToTraceType(ECC_WorldStatic),
        false, ActorsToIgnore, EDrawDebugTrace::ForDuration, LedgeHit, true
    );

    if (!bHitLedge || !LedgeHit.bBlockingHit) return false;

    OutResult.bCanClimb = true;
    OutResult.WallLocation = WallHit.ImpactPoint;
    OutResult.WallNormal = WallHit.ImpactNormal;
    OutResult.LedgeLocation = LedgeHit.ImpactPoint;

    const float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    OutResult.LandingLocation = LedgeHit.ImpactPoint + (FVector::UpVector * CapsuleHalfHeight);

    return true;
}
