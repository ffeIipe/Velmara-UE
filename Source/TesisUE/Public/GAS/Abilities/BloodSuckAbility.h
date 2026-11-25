#pragma once

#include "CoreMinimal.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "BloodSuckAbility.generated.h"

class UAnimMontage;
class UGameplayEffect;
class AEntity;
class AOrbBase;

UCLASS()
class TESISUE_API UBloodSuckAbility : public UVelmaraGameplayAbility
{
	GENERATED_BODY()

public:
	UBloodSuckAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	float GetGrabRange() const { return GrabRange; }
	AEntity* FindVictim(const AEntity* Self) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TObjectPtr<UAnimMontage> SuckMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TSubclassOf<UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float GrabRange = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float LungeDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	FName SelfBiteBone = TEXT("head");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	FName VictimBiteBone = TEXT("neck_01");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	FVector BiteOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	FName VictimHipBone = TEXT("pelvis");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TArray<FName> SelfBodyBones = {TEXT("spine_03"), TEXT("pelvis")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	float BodyClearance = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck")
	TObjectPtr<UAnimMontage> VictimReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck|Feedback")
	FGameplayTag BleedCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck|Feedback")
	TSubclassOf<AOrbBase> BloodOrbClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BloodSuck|Feedback", meta = (ClampMin = "0"))
	int32 BloodPulseCount = 6;

private:
	UPROPERTY()
	TWeakObjectPtr<AEntity> Victim;

	bool bIgnoringMoveInput = false;
	bool bAttached = false;
	FVector LungeStartLocation = FVector::ZeroVector;
	FRotator LungeStartRotation = FRotator::ZeroRotator;
	float LungeStartTime = 0.f;
	FTimerHandle AttachmentTimer;
	FTimerHandle BloodPulseTimer;

	void SetVictimFrozen(bool bFrozen) const;
	void TickAttachment();
	void EmitBloodPulse() const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();
};
