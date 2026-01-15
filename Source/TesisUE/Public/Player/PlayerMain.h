#pragma once

#include "CoreMinimal.h"
#include "Entities/Entity.h"
#include "PlayerMain.generated.h"


class UChangeModeComponent;

UCLASS()
class TESISUE_API APlayerMain : public AEntity
{
	GENERATED_BODY()

public:
	APlayerMain();
	
	//virtual float TakeDamage(
	//	float DamageAmount,
	//	FDamageEvent const& DamageEvent,
	//	AController* EventInstigator,
	//	AActor* DamageCauser) override;
	
	virtual bool IsPossessed() { return true; } //true by default, because it wouldn't be marked as a threat if not
	
	// --- State & Interaction ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State | Damage")
	bool bCanReceiveDamage = true;

	// --- Camera ---
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetFollowCamera();

	// --- Forms ---
	//UFUNCTION(BlueprintCallable, Category = "Gameplay | Forms")
	//void ToggleForm();

protected:
	virtual void OnRep_PlayerState() override;
	
private:
	// --- Life Cycle ---
	virtual void PerformDeath() override;
	
	void Revive();
	
	void LoadLastCheckpoint() const;
	
	//void ApplyModeConfig(const ECharacterModeStates& Mode);

	//UFUNCTION()
	//void ApplyHumanMode();

	//UFUNCTION()
	//void ApplySpectralMode();
};