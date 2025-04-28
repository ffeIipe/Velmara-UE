// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MementoComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UMementoComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMementoComponent();

	void SaveState();

	void LoadState();

	//useful when player falls
	FORCEINLINE FTransform GetLastTransform() { return CurrentMementoState.Transform; };

protected:
	virtual void BeginPlay() override;

private:	
	struct FMementoState
	{
		FTransform Transform;
		float Health;
		float Energy;
	};

	FMementoState CurrentMementoState;

	FMementoState GetCurrentEntityState(); 

	void ApplyEntityState(const FMementoState& StateToApply);
};