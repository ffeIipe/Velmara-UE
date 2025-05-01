// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Sword.generated.h"

class UBoxComponent;
class ICharacterState;
class UCharacterStateComponent;

UCLASS()
class TESISUE_API ASword : public AItem
{
	GENERATED_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);

	UPROPERTY(BlueprintAssignable, Category = "Collision")
	FOnWallHitSignature OnWallHit;

	struct FSwordStats
	{
		FName Name;
		FImage Image;
	};

	ASword();

	virtual void BeginPlay() override;

	void AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName);

	void Enable(bool Param);
	
	FORCEINLINE UBoxComponent* GetWeaponBox() const { return WeaponBox; }

	TArray<AActor*> IgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UDamageType> DamageTypeClass;

	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;
	
	virtual void Unequip() override;
	
	virtual void EnableVisuals(bool bEnable) override;
	
	virtual UPrimitiveComponent* GetCollisionComponent() override;

protected:
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable)
	void OnBoxOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent)
	void CameraShake();

private:
	ICharacterState* CharacterStateInterface = nullptr;

	UCharacterStateComponent* CharacterStateComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* WeaponBox;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* BoxTraceStart;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* BoxTraceEnd;

	UPROPERTY(EditAnywhere)
	float Damage;

	void OnWallCollision(const FHitResult& Hit);
};