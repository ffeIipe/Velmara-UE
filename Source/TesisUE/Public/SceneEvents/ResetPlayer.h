// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResetPlayer.generated.h"

class UBoxComponent;

UCLASS()
class TESISUE_API AResetPlayer : public AActor
{
	GENERATED_BODY()
	
public:	
	AResetPlayer();

protected:
	virtual void BeginPlay() override;
	
private:
	UPROPERTY(EditAnywhere)
	UBoxComponent* BoxCollider;

	UFUNCTION()
	void OnBoxOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	class APlayerStart* PlayerStart;
};
