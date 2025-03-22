// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;

UCLASS()
class TESISUE_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectile();
protected:
	virtual void BeginPlay() override;
	//virtual void Tick(DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Projectile | Collider")
	UBoxComponent* BoxCollider;

	UPROPERTY(EditAnywhere, Category = "Projectile | Mesh")
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile | ProjectileMovement")
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Stats")
	float Speed = 500.f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Stats")
	float Gravity = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Projectile | Stats")
	bool IsHoming = false;

	UFUNCTION()
	virtual void OnBoxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);


private:	
	void OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit);
};
