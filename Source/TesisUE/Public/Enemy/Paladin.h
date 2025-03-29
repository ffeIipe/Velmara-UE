// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Enemy.h"
#include "Paladin.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API APaladin : public AEnemy
{
	GENERATED_BODY()

public:
	APaladin();

protected:
	void BeginPlay();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* SwordMesh;
	
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* SwordBoxCollider;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceStart;
	
	UPROPERTY(EditDefaultsOnly)
	USceneComponent* BoxTraceEnd;

	UFUNCTION()
	virtual void OnBoxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	
};
