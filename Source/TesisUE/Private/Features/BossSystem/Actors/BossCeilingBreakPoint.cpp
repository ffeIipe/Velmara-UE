#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"

#include "Components/ArrowComponent.h"

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

	OnCeilingBroken();
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
