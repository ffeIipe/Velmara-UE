// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Interfaces/Weapon/MeleeWeapon.h"
#include "Items/Item.h"
#include "Sword.generated.h"

class IAnimatorProvider;
class ICharacterStateProvider;
class UNiagaraSystem;
class UBoxComponent;
class ICharacterState;
class UCharacterStateComponent;

UCLASS()
class TESISUE_API ASword : public AWeapon, public IMeleeWeapon
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);

	UPROPERTY(BlueprintAssignable, Category = "Collision")
	FOnWallHitSignature OnWallHit;

	ASword();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName) const;
	
	UBoxComponent* GetWeaponDamageBox() const { return WeaponDamageBox; }

	UPROPERTY()
	TArray<AActor*> IgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties | Damage")
	TSubclassOf<UDamageType> DamageTypeClass;

	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator) override;
	
	virtual void Unequip() override;

	UFUNCTION(BlueprintCallable)
	void EnableSwordState(bool bEnable) const;
	
	virtual UPrimitiveComponent* GetCollisionComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties | LightAttackAnims")
	TArray<UAnimMontage*> LightAttackCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties | HeavyAttackAnims")
	TArray<UAnimMontage*> HeavyAttackCombo;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties | JumpAttackAnims")
	TArray<UAnimMontage*> JumpAttackCombo;

	virtual int32 GetLightAttackComboMaxIndex() override;
	virtual int32 GetHeavyAttackComboMaxIndex() override;
	virtual int32 GetJumpAttackComboMaxIndex() override;
	
protected:
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

	virtual void UsePrimaryAttack() override;
	
private:
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | FX | SFX")
	USoundBase* ShieldImpactSFX;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties | FX | SFX")
	UNiagaraSystem* HitEffect;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | FX | VFX")
	UNiagaraSystem* SparksEffect;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties | Mesh | Attachment Socket")
	FName CustomInSocketName;

	TScriptInterface<ICharacterStateProvider> OwnerCharacterState;
	
	TScriptInterface<IAnimatorProvider> OwnerAnimator;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UBoxComponent* WeaponDamageBox;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* BoxTraceStart;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* BoxTraceEnd;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float Damage;
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	float CriticalChance = .15f;
	
	UPROPERTY(EditAnywhere, Category = "Damage")
	float CriticalDamageMultiplier = 2.f;

	float CalculateDamage() const;

	void OnWallCollision(const FHitResult& Hit);

	virtual void PerformLightAttack(int32 ComboIndex) override;
	virtual void PerformHeavyAttack(int32 ComboIndex) override;
	virtual void PerformJumpAttack(int32 ComboIndex) override;

	virtual void ClearIgnoreActors() override { IgnoreActors.Empty(); }
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;
};