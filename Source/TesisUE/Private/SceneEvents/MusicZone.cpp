#include "SceneEvents/MusicZone.h"

#include "EngineUtils.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace MusicZone
{
	constexpr float InterruptFadeSeconds = 1.f;
}

AMusicZone::AMusicZone()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(200.f));
	Box->SetCollisionProfileName(TEXT("Trigger"));
	SetRootComponent(Box);

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Box);
	Audio->bAutoActivate = false;
	Audio->bAllowSpatialization = false;
}

void AMusicZone::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AMusicZone::HandleBoxOverlap);
	GetWorldTimerManager().SetTimerForNextTick(this, &AMusicZone::StartIfPlayerInside);
}

void AMusicZone::StartIfPlayerInside()
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (bStartOnPlayerOverlap && Pawn && Box->IsOverlappingActor(Pawn))
	{
		StartMusic();
	}
}

void AMusicZone::HandleBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (bStartOnPlayerOverlap && Pawn && Pawn->IsPlayerControlled())
	{
		StartMusic();
	}
}

void AMusicZone::ReceiveSignal_Implementation(const bool bActive, AActor* Activator)
{
	if (bActive)
	{
		ActiveSources.Add(Activator);
		StartMusic();
		return;
	}

	ActiveSources.Remove(Activator);
	if (ActiveSources.IsEmpty())
	{
		StopMusic();
	}
}

bool AMusicZone::IsMusicActive() const
{
	return bPlaying && Audio->IsPlaying();
}

void AMusicZone::StartMusic()
{
	if (!Music || IsMusicActive() || (bPlayOnce && bHasPlayed)) return;

	if (!bInterruptsOtherMusic)
	{
		for (TActorIterator<AMusicZone> It(GetWorld()); It; ++It)
		{
			if (It->bInterruptsOtherMusic && It->IsMusicActive()) return;
		}
	}
	else
	{
		for (TActorIterator<AMusicZone> It(GetWorld()); It; ++It)
		{
			if (*It != this)
			{
				It->FadeOutOver(MusicZone::InterruptFadeSeconds);
			}
		}
	}

	bPlaying = true;
	bHasPlayed = true;
	Audio->SetSound(Music);
	Audio->FadeIn(FadeInSeconds);

	if (MaxDurationSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(DurationTimer, this, &AMusicZone::StopMusic, MaxDurationSeconds, false);
	}
}

void AMusicZone::StopMusic()
{
	FadeOutOver(FadeOutSeconds);
}

void AMusicZone::FadeOutOver(const float Seconds)
{
	if (!bPlaying) return;

	bPlaying = false;
	GetWorldTimerManager().ClearTimer(DurationTimer);
	Audio->FadeOut(Seconds, 0.f);
}
