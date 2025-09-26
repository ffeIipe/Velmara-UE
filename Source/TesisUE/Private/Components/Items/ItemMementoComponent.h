// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemMementoComponent.generated.h"

USTRUCT(BlueprintType)
struct FItemMementoState
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName UniqueSaveID;

	UPROPERTY(SaveGame)
	bool bWasOpened;

	FItemMementoState()
		: UniqueSaveID(NAME_None)
		, bWasOpened(false)
	{}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UItemMementoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemMementoComponent();

	FItemMementoState CaptureItemState();
	
	FItemMementoState GetInternalItemState() const { return InternalItemState; }

	void ApplyExternalState(FItemMementoState ItemStateToApply);
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(SaveGame)
	FItemMementoState InternalItemState;
};
