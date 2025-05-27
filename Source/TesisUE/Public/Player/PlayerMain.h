// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterStates.h"
#include "Interfaces/FormInterface.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/CharacterState.h"
#include "Interfaces/MementoEntity.h"
#include "PlayerMain.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AEnemy;
class AItem;
class AWeapon;
class UAttributeComponent;
class UBoxComponent;
class UMementoComponent;
class UCombatComponent;
class UInventoryComponent;
class UCharacterStateComponent;
class UPlayerFormComponent;
class UTimelineComponent;
class UCameraComponent;
class USpringArmComponent;
class USpectralWeaponComponent;

UCLASS()
class TESISUE_API APlayerMain : public ACharacter, public IHitInterface, public ICharacterState, public IFormInterface, public IMementoEntity
{
	GENERATED_BODY()

public:
	APlayerMain();

	virtual void PerformSpectralAttack_Implementation() override;

	virtual void PerformSpectralBarrier_Implementation() override;

	virtual void ResetSpectralAttack_Implementation() override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType) override;

	virtual UCharacterStateComponent* GetCharacterStateComponent_Implementation() override;

	virtual UMementoComponent* GetMementoComponent_Implementation() override;

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
	void ReleasePossession(AEnemy* EnemyPossessed);

	UFUNCTION(BlueprintCallable)
	void ResetFollowCamera();

	UFUNCTION(BlueprintCallable, Category = "Forms")
	void ToggleForm();

protected:
	/*
	* Base
	*/
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	float InteractTraceLenght;

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* DodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Montages | Dodge")
	UAnimMontage* SpectralDodgeMontage;
	
	UPROPERTY(EditAnywhere, Category = "Montages | DoubleJump")
	UAnimMontage* DoubleJumpMontage;
	
	UPROPERTY(EditAnywhere, Category = "Montages | Death")
	UAnimMontage* DeathMontage;
	
	UPROPERTY(EditAnywhere, Category = "Montages | Jump")
	UAnimMontage* JumpMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Dodge")
	bool bIsSaveDodge = false;

	UFUNCTION()
	void Dodge();

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void PerformDodge();

	public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Forms")
	UPlayerFormComponent* PlayerFormComponent;

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

	UPROPERTY(BlueprintReadWrite, Category = "SpectralMode | SpectralAttack")
	int SpectralAttackIndex = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Montages | SpectralMode | SpectralAttack")
	TArray<UAnimMontage*> SpectralAttackCombo;

	UPROPERTY(BlueprintReadOnly, Category = "Montages | SpectralMode | SpectralAttack")
	UAnimMontage* SpectralHeavyAttack;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	AEnemy* SpectralTarget;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetDistance;

	UPROPERTY(EditAnywhere, Category = "SpectralAttack")
	float TrackTargetRadius;
	
	UPROPERTY(EditAnywhere, Category = "Interact")
	float InteractTargetRadius;

	UFUNCTION(BlueprintCallable, Category = "SpectralAttack")
	void SearchTarget();

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
	
	UPROPERTY(EditAnywhere, Category = "Input | Actions")
	UInputAction* InventoryAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere);
	UAttributeComponent* Attributes;

	UPROPERTY(VisibleAnywhere);
	UMementoComponent* MementoComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCharacterStateComponent* CharacterStateComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpectralWeaponComponent* SpectralWeaponComponent;

	UFUNCTION()
	void OnWallCollision(const FHitResult& HitResult);

private:
	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	USoundBase* ErrorSFX;
	
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

	void Die();

	UFUNCTION()
	void WithEnergy();
	
	UFUNCTION()
	void OutOfEnergy();

	UFUNCTION()
	void RestartLevel();
	
	UFUNCTION()
	void GoToMainMenu();

	void Revive();

	void LoadLastCheckpoint();

	void ChangePrimaryWeapon();
	
	void ChangeSecondaryWeapon();
};