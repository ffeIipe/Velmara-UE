// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Trigger.generated.h"

class APlayerMain;
class UBoxComponent;

UCLASS()
class TESISUE_API ATrigger : public AActor
{
	GENERATED_BODY()
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerBeginOverlap);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEndOverlap);

public:	
	ATrigger();

	FOnPlayerBeginOverlap OnPlayerBeginOverlap;
	FOnPlayerEndOverlap OnPlayerEndOverlap;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BoxCollider;

	UFUNCTION()
	virtual void DisableCollision();

	APlayerMain* Player;
};
