// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterStates.h"
#include "Interfaces/FormInterface.h"
#include "Interfaces/HitInterface.h"
#include "PlayerMain.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UTimelineComponent;
class AItem;
class ASword;
class UPlayerFormComponent;
class AEnemy;
class ASpectralObjectInteractable;
class UAttributeComponent;
class UEnergy;
class UBoxComponent;
class UMementoComponent;
class UCombatComponent;
class UInventoryComponent;

UCLASS()
class TESISUE_API APlayerMain : public ACharacter, public IFormInterface, public IHitInterface
{
	GENERATED_BODY()

public:
	APlayerMain();

	virtual ECharacterForm GetCharacterForm_Implementation() override;

	virtual void PerformSpectralAttack_Implementation() override;

	virtual void PerformSpectralBarrier_Implementation() override;

	virtual void ResetSpectralAttack_Implementation() override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere)
	ACameraActor* FollowCamera;

	UFUNCTION(BlueprintPure, Category = "SpectralAttack")
	FORCEINLINE AEnemy* GetSpectralTarget() const { return SpectralTarget; }
	
	FORCEINLINE void SetOverlappingItem(AItem* Item) { OverlappingItem = Item; }

	UFUNCTION()
	FORCEINLINE UAttributeComponent* GetAttributes() { return Attributes; };

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled);

	UPROPERTY(VisibleAnywhere, Category = "Forms")
	UPlayerFormComponent* PlayerFormComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool CanDoubleJump = true;

	/*
	*  TakeDamage
	*/
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	bool bCanReceiveDamage = true;

	UFUNCTION(BlueprintCallable)
	void ReleasePossession();

	UFUNCTION(BlueprintCallable)
	void ResetFollowCamera();


protected:
	/*
	* Base
	*/
	virtual void BeginPlay() override;

	/*
	* Interact
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpectralMode")
	float SpectralInteractDistance;

	/*
	* Dodge
	*/
	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	UAnimMontage* SpectralDodgeMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "DoubleJump")
	UAnimMontage* DoubleJumpMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Death")
	UAnimMontage* DeathMontage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Jump")
	UAnimMontage* JumpMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bSaveDodge = false;

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

	public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UCombatComponent* CombatComponent;

	UPROPERTY()
	UTimelineComponent* BufferDodgeTimeline;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	class UCurveFloat* BufferCurve;

	UPROPERTY(EditAnywhere, Category = "Buffer")
	float BufferDodgeDistance;

	UFUNCTION(BlueprintCallable)
	void DodgeBufferEvent(float BufferAmount);

	UFUNCTION(BlueprintCallable)
	void StopDodgeBufferEvent();

	UFUNCTION(BlueprintCallable)
	void UpdateDodgeBuffer(float Alpha);

	UFUNCTION(BlueprintCallable)
	void UpdateBuffer(float Alpha, float BufferDistance);

	/*
	* Light Attack
	*/
	UPROPERTY(BlueprintReadWrite, Category = "SpectralMode | SpectralAttack")
	int SpectralAttackIndex = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "SpectralMode | SpectralAttack")
	TArray<UAnimMontage*> SpectralAttackCombo;
	
	/*
	* Heavy Attack
	*/
	UPROPERTY(BlueprintReadOnly, Category = "SpectralMode | SpectralAttack")
	UAnimMontage* SpectralHeavyAttack;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	AEnemy* SpectralTarget;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetDistance;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetRadius;

	UFUNCTION(BlueprintCallable, Category = "SpectralAttack")
	void SearchTarget();

	/*
	* Inputs
	*/
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* CharacterContext;

	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* DodgeAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Movement")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* ChangeFormAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* PossessAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* RestartAction;
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* GoToMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere);
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere);
	UMementoComponent* MementoComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly);
	UInventoryComponent* InventoryComponent;

private:
	UPROPERTY();
	EMainDamageTypes LastDamageType;

	UPROPERTY(EditDefaultsOnly, Category = "BloodSense | Cooldown")
	float TransformationCooldown;

	UPROPERTY(EditAnywhere, Category = "BloodSense | Cooldown")
	float LastTransformationTime;
	
	UPROPERTY(EditAnywhere, Category = "Possess")
	float PossessDistance;

	class APlayerController* PlayerControllerRef = nullptr;

	bool bIsDead = false;

	UPROPERTY(VisibleInstanceOnly)
	AItem* OverlappingItem;	
	
	UFUNCTION(BlueprintCallable)
	void HitStop(float Duration, float TimeScale);
	
	UFUNCTION(BlueprintCallable)
	void ResetTimeDilation();
	
	bool bIsPossessing = false;

	AEnemy* PossessedEnemy;

	UFUNCTION(BlueprintCallable)
	AEnemy* GetTargetEnemy();
	
	UFUNCTION(BlueprintCallable)
	void PossessEnemy();

	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void Jump() override;

	void DoubleJump();

	void Landed(const FHitResult& Hit) override;

	void Interact(const FInputActionValue& Value);
	
	void Attack(const FInputActionValue& Value);

	void HeavyAttack(const FInputActionValue& Value);
	
	void LaunchAttack(const FInputActionValue& Value);

	void Block(const FInputActionValue& Value);

	void ReleaseBlock(const FInputActionValue& Value);
	
	void Execute(const FInputActionValue& Value);

	void ToggleForm();

	void Die();

	void SwitchWeapon();

	UFUNCTION()
	void WithEnergy();
	
	UFUNCTION()
	void OutOfEnergy();

	UFUNCTION()
	void RestartLevel();
	
	UFUNCTION()
	void GoToMainMenu();

	UFUNCTION()
	void OnWallCollision(const FHitResult& HitResult);

	void Revive();

	void LoadLastCheckpoint();
};