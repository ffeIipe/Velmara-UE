#include "Components/MementoComponent.h"
#include "Components/AttributeComponent.h"
#include <Kismet/GameplayStatics.h>
#include "GameFramework/PlayerStart.h"

UMementoComponent::UMementoComponent()
{	
	PrimaryComponentTick.bCanEverTick = false;
}

void UMementoComponent::SaveState()
{
	if (GetOwner())
	{
		CurrentMementoState = GetCurrentEntityState();
	}
}

void UMementoComponent::LoadState()
{
	if (GetOwner())
	{
		ApplyEntityState(CurrentMementoState);
	}
}

void UMementoComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerStart = Cast<APlayerStart>(UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()));
}

UMementoComponent::FMementoState UMementoComponent::GetCurrentEntityState()
{
	FMementoState State;
	if (AActor* Owner = GetOwner())
	{
		State.Transform = Owner->GetActorTransform();
		State.Health = Owner->GetComponentByClass<UAttributeComponent>()->GetHealth();
		State.Energy = Owner->GetComponentByClass<UAttributeComponent>()->GetEnergy();
		
		PlayerStart->SetActorTransform(Owner->GetActorTransform());
	}
	return State;
}

void UMementoComponent::ApplyEntityState(const FMementoState& StateToApply)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorTransform(StateToApply.Transform);
		Owner->GetComponentByClass<UAttributeComponent>()->SetHealth(StateToApply.Health);
		Owner->GetComponentByClass<UAttributeComponent>()->SetEnergy(StateToApply.Energy);
	}
}

