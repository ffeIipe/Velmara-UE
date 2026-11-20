#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"

#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace BossCeilingRock
{
	constexpr float MaxFallTime = 3.f;
}

ABossCeilingBreakPoint::ABossCeilingBreakPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Root);
	Arrow->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Arrow->ArrowColor = FColor(255, 220, 120);
	Arrow->ArrowSize = 3.f;
	Arrow->bHiddenInGame = true;

	Rock = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("Rock"));
	Rock->SetupAttachment(Root);
	Rock->SetMobility(EComponentMobility::Movable);
	Rock->SetSimulatePhysics(true);
	Rock->bNotifyCollisions = true;
	Rock->SetCollisionProfileName(TEXT("Destructible"));
	Rock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Rock->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Rock->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<UGeometryCollection> RockFinder(TEXT("/Game/Blueprints/Entities/Enemies/Boss/GC_BossCeilingSlab.GC_BossCeilingSlab"));
	if (RockFinder.Succeeded())
	{
		Rock->RestCollection = RockFinder.Object;
	}
}

void ABossCeilingBreakPoint::BeginPlay()
{
	Super::BeginPlay();

	if (Rock && Rock->GetRestCollection())
	{
		Rock->SetAnchoredByBox(Rock->Bounds.GetBox().ExpandBy(500.f), true);
	}
}

void ABossCeilingBreakPoint::Break()
{
	if (bBroken) return;

	bBroken = true;

	for (AActor* Piece : CeilingPiecesToHide)
	{
		if (IsValid(Piece))
		{
			Piece->SetActorHiddenInGame(true);
			Piece->SetActorEnableCollision(false);
		}
	}

	if (Rock && Rock->GetRestCollection())
	{
		Rock->OnChaosPhysicsCollision.AddUniqueDynamic(this, &ABossCeilingBreakPoint::HandleRockCollision);
		Rock->RemoveAllAnchors();
		GetWorldTimerManager().SetTimer(SettleTimer, this, &ABossCeilingBreakPoint::Shatter, BossCeilingRock::MaxFallTime, false);
	}

	OnCeilingBroken();
}

void ABossCeilingBreakPoint::HandleRockCollision(const FChaosPhysicsCollisionInfo& CollisionInfo)
{
	Shatter();
}

void ABossCeilingBreakPoint::Shatter()
{
	if (bShattered) return;

	bShattered = true;
	Rock->OnChaosPhysicsCollision.RemoveDynamic(this, &ABossCeilingBreakPoint::HandleRockCollision);

	Rock->CrumbleActiveClusters();

	GetWorldTimerManager().SetTimer(SettleTimer, this, &ABossCeilingBreakPoint::Settle, SettleTime, false);
}

void ABossCeilingBreakPoint::Settle()
{
	Rock->ApplyKinematicField(Rock->Bounds.SphereRadius * 4.f + 1000.f, Rock->Bounds.Origin);
	Rock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void ABossCeilingBreakPoint::ResetPoint()
{
	bBroken = false;

	for (AActor* Piece : CeilingPiecesToHide)
	{
		if (IsValid(Piece))
		{
			Piece->SetActorHiddenInGame(false);
			Piece->SetActorEnableCollision(true);
		}
	}
}
