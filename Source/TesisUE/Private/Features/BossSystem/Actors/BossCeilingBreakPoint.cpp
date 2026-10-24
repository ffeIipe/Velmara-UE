#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"

#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace BossCeilingRock
{
	// Si por algún motivo no llega el evento de impacto, se parte igual pasado este tiempo de caída.
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
	// Simula desde el inicio pero anclada (BeginPlay): así queda quieta en el techo hasta que se rompe.
	Rock->SetSimulatePhysics(true);
	Rock->bNotifyCollisions = true;
	Rock->SetCollisionProfileName(TEXT("Destructible"));
	// Mientras simula, nadie la puede empujar (ni jugador, ni enemigos, ni el boss): los pawns la atraviesan.
	// Al asentarse pasa a bloquear Pawn y queda estática.
	Rock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Rock->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// Los traces de techo/piso y de línea de sol usan Visibility: la roca no tiene que taparlos.
	Rock->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// Roca fracturada por defecto (5 pedazos). Se puede reemplazar en el Blueprint.
	static ConstructorHelpers::FObjectFinder<UGeometryCollection> RockFinder(TEXT("/Game/Blueprints/Entities/Enemies/Boss/GC_BossCeilingRock.GC_BossCeilingRock"));
	if (RockFinder.Succeeded())
	{
		Rock->RestCollection = RockFinder.Object;
	}
}

void ABossCeilingBreakPoint::BeginPlay()
{
	Super::BeginPlay();

	// shortcut: BP_BossCeilingRock nació con Rock como StaticMesh y su CDO guardó el puntero en null; recrear el BP elimina esto.
	if (!Rock)
	{
		Rock = FindComponentByClass<UGeometryCollectionComponent>();
	}

	// El GC no pasa de estático a dinámico con SetSimulatePhysics en runtime: se ancla y Break quita los anclajes.
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

	// Rock es null en instancias guardadas antes de que la roca fuera un Geometry Collection.
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

	// Se parte en los pedazos del fracturado (máximo 5) al tocar el suelo.
	Rock->CrumbleActiveClusters();

	GetWorldTimerManager().SetTimer(SettleTimer, this, &ABossCeilingBreakPoint::Settle, SettleTime, false);
}

void ABossCeilingBreakPoint::Settle()
{
	// Todos los pedazos pasan a cinemáticos: quedan donde cayeron, sin física.
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
