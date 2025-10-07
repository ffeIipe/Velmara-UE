// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Door.generated.h"

UCLASS()
class TESISUE_API ADoor : public AActor
{
	GENERATED_BODY()

public:
	ADoor();

	UFUNCTION(BlueprintImplementableEvent)
	void OpenDoor();
	
	UFUNCTION(BlueprintImplementableEvent)
	void CloseDoor();
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* DoorMeshComponent;
};
