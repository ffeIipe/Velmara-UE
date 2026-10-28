#include "Features/TotemSystem/Core/TotemChargeComponent.h"

#include "GenericTeamAgentInterface.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Entities/Entity.h"
#include "Features/TotemSystem/Subsystems/TotemSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UTotemChargeComponent::UTotemChargeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTotemChargeComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentKills = bStartCharged ? KillsToCharge : 0;

	if (UTotemSubsystem* TotemSubsystem = GetWorld()->GetSubsystem<UTotemSubsystem>())
	{
		TotemSubsystem->RegisterTotem(this);
	}

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UTotemChargeComponent::HandleOwnerControllerChanged);
	}

	// Se aplica en el siguiente frame para no pisarse con el BeginPlay del dueño (MIDs de AEntity, perfiles de colisión del Blueprint).
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		RefreshPossessionTraceResponse();
		RefreshChargeMaterials();
	}));
}

void UTotemChargeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		if (UTotemSubsystem* TotemSubsystem = World->GetSubsystem<UTotemSubsystem>())
		{
			TotemSubsystem->UnregisterTotem(this);
		}
	}

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UTotemChargeComponent::HandleOwnerControllerChanged);
	}

	Super::EndPlay(EndPlayReason);
}

bool UTotemChargeComponent::CanActorBePossessed(const AActor* Actor)
{
	if (!Actor) return false;

	const UTotemChargeComponent* ChargeComponent = Actor->FindComponentByClass<UTotemChargeComponent>();

	return !ChargeComponent || ChargeComponent->IsCharged();
}

bool UTotemChargeComponent::IsCharged() const
{
	return !bRequiresCharge || CurrentKills >= KillsToCharge;
}

float UTotemChargeComponent::GetChargePercent() const
{
	if (!bRequiresCharge) return 1.f;

	return FMath::Clamp(static_cast<float>(CurrentKills) / FMath::Max(KillsToCharge, 1), 0.f, 1.f);
}

bool UTotemChargeComponent::TryAbsorbDeath(AActor* DeadActor)
{
	if (!bRequiresCharge || !DeadActor || DeadActor == GetOwner()) return false;

	// Ya cargado, o el jugador está dentro del tótem: las muertes no suman.
	if (IsCharged() || IsOwnerPlayerControlled()) return false;

	if (bOnlyCountEnemies && !IsEnemyOfPlayer(DeadActor)) return false;

	const float DistanceSquared = FVector::DistSquared(GetOwner()->GetActorLocation(), DeadActor->GetActorLocation());
	if (DistanceSquared > FMath::Square(ChargeRadius)) return false;

	OnEnemyAbsorbed.Broadcast(DeadActor);

	AddCharge(1);

	return true;
}

void UTotemChargeComponent::AddCharge(const int32 Kills)
{
	if (!bRequiresCharge || Kills <= 0) return;

	SetCurrentKills(CurrentKills + Kills);
}

void UTotemChargeComponent::FillCharge()
{
	if (!bRequiresCharge) return;

	SetCurrentKills(KillsToCharge);
}

void UTotemChargeComponent::ConsumeCharge()
{
	if (!bRequiresCharge) return;

	SetCurrentKills(0);
}

void UTotemChargeComponent::SetRequiresCharge(const bool bNewRequiresCharge)
{
	if (bRequiresCharge == bNewRequiresCharge) return;

	const bool bWasCharged = IsCharged();

	bRequiresCharge = bNewRequiresCharge;

	NotifyChargeChanged(bWasCharged);
}

void UTotemChargeComponent::SetCurrentKills(const int32 NewKills)
{
	const int32 ClampedKills = FMath::Clamp(NewKills, 0, KillsToCharge);
	if (ClampedKills == CurrentKills) return;

	const bool bWasCharged = IsCharged();

	CurrentKills = ClampedKills;

	NotifyChargeChanged(bWasCharged);
}

void UTotemChargeComponent::NotifyChargeChanged(const bool bWasCharged)
{
	RefreshPossessionTraceResponse();
	RefreshChargeMaterials();

	OnChargeChanged.Broadcast(CurrentKills, KillsToCharge);

	const bool bIsChargedNow = IsCharged();

	if (!bWasCharged && bIsChargedNow)
	{
		OnFullyCharged.Broadcast();
	}
	else if (bWasCharged && !bIsChargedNow)
	{
		OnChargeConsumed.Broadcast();
	}
}

void UTotemChargeComponent::HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (bConsumeChargeOnPossession && NewController && NewController->IsPlayerController())
	{
		ConsumeCharge();
	}
}

bool UTotemChargeComponent::IsOwnerPlayerControlled() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());

	return OwnerPawn && OwnerPawn->IsPlayerControlled();
}

bool UTotemChargeComponent::IsEnemyOfPlayer(const AActor* DeadActor) const
{
	const AEntity* DeadEntity = Cast<AEntity>(DeadActor);
	if (!DeadEntity || DeadEntity->IsPlayerControlled()) return false;

	const IGenericTeamAgentInterface* PlayerTeamAgent = Cast<IGenericTeamAgentInterface>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerTeamAgent) return true;

	return DeadEntity->GetTeamId() != PlayerTeamAgent->GetGenericTeamId();
}

void UTotemChargeComponent::RefreshPossessionTraceResponse()
{
	if (!bIgnorePossessionTraceWhileUncharged || !GetOwner()) return;

	const bool bShouldIgnoreTrace = !IsCharged();

	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Primitive : Primitives)
	{
		const TWeakObjectPtr<UPrimitiveComponent> PrimitiveKey(Primitive);

		if (bShouldIgnoreTrace)
		{
			if (!CachedTraceResponses.Contains(PrimitiveKey))
			{
				CachedTraceResponses.Add(PrimitiveKey, Primitive->GetCollisionResponseToChannel(PossessionTraceChannel));
			}

			Primitive->SetCollisionResponseToChannel(PossessionTraceChannel, ECR_Ignore);
		}
		else if (const TEnumAsByte<ECollisionResponse>* CachedResponse = CachedTraceResponses.Find(PrimitiveKey))
		{
			Primitive->SetCollisionResponseToChannel(PossessionTraceChannel, *CachedResponse);
		}
	}

	if (!bShouldIgnoreTrace)
	{
		CachedTraceResponses.Reset();
	}
}

void UTotemChargeComponent::RefreshChargeMaterials() const
{
	if (ChargeMaterialParameter.IsNone() || !GetOwner()) return;

	TArray<UMeshComponent*> Meshes;
	GetOwner()->GetComponents<UMeshComponent>(Meshes);

	const float ChargePercent = GetChargePercent();

	for (UMeshComponent* Mesh : Meshes)
	{
		// El widget de posesión también es un UMeshComponent, pero maneja sus propios materiales.
		if (Mesh->IsA<UWidgetComponent>()) continue;

		Mesh->SetScalarParameterValueOnMaterials(ChargeMaterialParameter, ChargePercent);
	}
}
