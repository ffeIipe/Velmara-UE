#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionSystem/Interfaces/SignalReceiver.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"
#include "LogicGate.generated.h"

UCLASS()
class TESISUE_API ALogicGate : public AActor, public ISignalReceiver, public ISaveInterface
{
	GENERATED_BODY()
	
public:	
	ALogicGate();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugConnections = true;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Properties")
	int32 RequiredActivations = 2;

	UPROPERTY(EditInstanceOnly, Category = "Properties")
	TArray<AActor*> LinkedTargets;

	UPROPERTY(SaveGame)
	TSet<AActor*> ActiveSources;

	UPROPERTY(SaveGame)
	bool bOutputState = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveGame")
	FName UniqueSaveID;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

public:
	virtual void ReceiveSignal_Implementation(bool bActive, AActor* Activator) override;

	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; }
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override; 
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;

private:
	void UpdateLogicState();
	void BroadcastToTargets(bool bActive);
};