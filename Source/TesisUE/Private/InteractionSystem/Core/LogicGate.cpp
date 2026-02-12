#include "InteractionSystem/Core/LogicGate.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

ALogicGate::ALogicGate()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALogicGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDebugConnections)
	{
		const FColor LineColor = bOutputState ? FColor::Green : FColor::Red;
        
		for (const AActor* Target : LinkedTargets)
		{
			if (Target)
			{
				UKismetSystemLibrary::DrawDebugArrow(GetWorld(), GetActorLocation(), Target->GetActorLocation(), 
					50.0f, LineColor, -1.0f, 2.0f);
			}
		}

		const FString DebugMsg = FString::Printf(TEXT("GATE: %d / %d"), ActiveSources.Num(), RequiredActivations);
		const FColor TextColor = (ActiveSources.Num() >= RequiredActivations) ? FColor::Green : FColor::Yellow;
        
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0,0,50), DebugMsg, nullptr, TextColor, 0.0f);
	}
}

void ALogicGate::BeginPlay()
{
	Super::BeginPlay();
	UpdateLogicState();
}

#if WITH_EDITOR
bool ALogicGate::ShouldTickIfViewportsOnly() const
{
	return bDebugConnections;
}
#endif

void ALogicGate::ReceiveSignal_Implementation(const bool bActive, AActor* Activator)
{
	if (!Activator) return;

	if (bActive)
	{
		ActiveSources.Add(Activator);
	}
	else
	{
		ActiveSources.Remove(Activator);
	}

	UpdateLogicState();
}

void ALogicGate::OnSaveGame_Implementation(FEntitySaveData& OutData)
{
	FMemoryWriter MemWriter(OutData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
	Ar.ArIsSaveGame = true;
    
	this->Serialize(Ar); 
}

void ALogicGate::OnLoadGame_Implementation(const FEntitySaveData& InData)
{
	FMemoryReader MemReader(InData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
	Ar.ArIsSaveGame = true;

	this->Serialize(Ar); 
}

void ALogicGate::UpdateLogicState()
{
	if (const bool bShouldBeActive = ActiveSources.Num() >= RequiredActivations; bOutputState != bShouldBeActive)
	{
		bOutputState = bShouldBeActive;
		BroadcastToTargets(bOutputState);
	}
}

void ALogicGate::BroadcastToTargets(const bool bActive)
{
	for (AActor* Target : LinkedTargets)
	{
		if (!Target) continue;
		
		if (Target->GetClass()->ImplementsInterface(USignalReceiver::StaticClass()))
		{
			Execute_ReceiveSignal(Target, bActive, this);
		}
	}
    
	if (bActive) DrawDebugString(GetWorld(), GetActorLocation(), "GATE OPEN", nullptr, FColor::Green, 2.0f);
}