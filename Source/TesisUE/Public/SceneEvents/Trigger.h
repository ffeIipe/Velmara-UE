// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Trigger.generated.h"

class IHitInterface;
class APlayerMain;
class UBoxComponent;

UCLASS()
class TESISUE_API ATrigger : public AActor
{
	GENERATED_BODY()
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerControllerBeginOverlap);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerControllerEndOverlap);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerBeginOverlap);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerEndOverlap);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityBeginOverlap);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityEndOverlap);

public:	
	ATrigger();

	UPROPERTY(BlueprintAssignable)
	FOnPlayerControllerBeginOverlap OnPlayerControllerBeginOverlap;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerControllerEndOverlap OnPlayerControllerEndOverlap;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerBeginOverlap OnPlayerBeginOverlap;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerEndOverlap OnPlayerEndOverlap;
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityBeginOverlap OnEntityBeginOverlap;
	
	UPROPERTY(BlueprintAssignable)
	FOnEntityEndOverlap OnEntityEndOverlap;

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
	
	TScriptInterface<IHitInterface> HitActor;
};
