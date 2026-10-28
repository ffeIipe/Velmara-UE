#include "Subsystems/SpectralObjectsSubsystem.h"
#include "SpectralMode/Interfaces/Spectral.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace Spectral
{
	constexpr float UpdateInterval = 0.05f;
	constexpr float WarningTime = 1.5f;   // Parpadeo antes de desaparecer.
	constexpr float LeaveGrace = 3.f;     // Tras bajarse: 1.5 s fijo + 1.5 s parpadeando.
	constexpr float FlickerPeriod = 0.25f;
	constexpr float TouchMargin = 30.f;
}


void USpectralObjectsSubsystem::AddSpectralObject(AActor* Actor)
{
	if (Actor->GetClass()->ImplementsInterface(USpectral::StaticClass()))
	{
		const TScriptInterface<ISpectral> SpectralInterface = Actor;
		SpectralObjects.AddUnique(SpectralInterface);

		// Stencil 4 = "SP Item" en M_BloodSenseV2: así el post process los ilumina durante el pulso.
		Actor->ForEachComponent<UPrimitiveComponent>(false, [](UPrimitiveComponent* Primitive)
		{
			Primitive->SetRenderCustomDepth(true);
			Primitive->SetCustomDepthStencilValue(4);
		});
	}
}

void USpectralObjectsSubsystem::ActivateSpectralObjectsInRange(const FVector& Origin, const float Radius, const float Duration)
{
	const double ExpireTime = GetWorld()->GetTimeSeconds() + Duration;

	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		AActor* Actor = Cast<AActor>(SpectralObject.GetObject());
		if (Actor && Actor->GetComponentsBoundingBox(true).ComputeSquaredDistanceToPoint(Origin) <= FMath::Square(Radius))
		{
			SpectralObject->Execute_ActivateVisibility(Actor);
			RevealedUntil.Add(Actor, ExpireTime);
		}
	}

	if (!RevealedUntil.IsEmpty() && !GetWorld()->GetTimerManager().IsTimerActive(UpdateTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &USpectralObjectsSubsystem::UpdateRevealedObjects, Spectral::UpdateInterval, true);
	}
}

void USpectralObjectsSubsystem::UpdateRevealedObjects()
{
	const double Now = GetWorld()->GetTimeSeconds();

	for (auto It = RevealedUntil.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Key.Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}

		// Nunca se le va el piso al jugador: el tiempo recién corre cuando se baja.
		if (IsPlayerUsing(Actor))
		{
			It->Value = FMath::Max(It->Value, Now + Spectral::LeaveGrace);
		}

		const double Remaining = It->Value - Now;
		if (Remaining <= 0.0)
		{
			ISpectral::Execute_DeactivateVisibility(Actor);
			It.RemoveCurrent();
		}
		else if (Remaining <= Spectral::WarningTime)
		{
			// Parpadeo de aviso: solo la visibilidad, la colisión sigue hasta el final.
			Actor->SetActorHiddenInGame(FMath::Fmod(Remaining, Spectral::FlickerPeriod) < Spectral::FlickerPeriod * 0.5);
		}
		else
		{
			Actor->SetActorHiddenInGame(false);
		}
	}

	if (RevealedUntil.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	}
}

bool USpectralObjectsSubsystem::IsPlayerUsing(const AActor* Actor) const
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player) return false;

	// Tocando el bounds con la cápsula cubre parado encima, colgado o trepando.
	const float Reach = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + Spectral::TouchMargin;
	return Actor->GetComponentsBoundingBox(true).ComputeSquaredDistanceToPoint(Player->GetActorLocation()) <= FMath::Square(Reach);
}

void USpectralObjectsSubsystem::ActivateSpectralObjects()
{
	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->Execute_ActivateVisibility(SpectralObject.GetObject());
	}
}

void USpectralObjectsSubsystem::DeactivateSpectralObjects()
{
	RevealedUntil.Reset();
	for (const TScriptInterface SpectralObject : SpectralObjects)
	{
		SpectralObject->Execute_DeactivateVisibility(SpectralObject.GetObject());
	}
}

void USpectralObjectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DeactivateSpectralObjects();
}