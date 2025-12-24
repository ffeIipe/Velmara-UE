#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class AEntity;
class IWeaponInterface;
struct FTargetingData;
class UTimelineComponent;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHardLockToggled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHardLockOn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHardLockOff);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetingComponent();

    void InitializeValues(const FTargetingData& TargetingData);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    UPROPERTY(BlueprintAssignable)
    FOnHardLockToggled OnHardLockToggled;

    UPROPERTY(BlueprintAssignable)
    FOnHardLockOn OnHardLockOn;
    
    UPROPERTY(BlueprintAssignable)
    FOnHardLockOff OnHardLockOff;

    UFUNCTION()
    void HandleTargetDeath(AEntity* DeadEntity);
    
    UFUNCTION()
    void HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon);
    
    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void ToggleHardLock();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void ChangeHardLockTarget();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void PerformSoftLock();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    AEntity* GetCurrentTarget() const { return CurrentTarget; }

    UFUNCTION()
    void RemoveCombatTarget();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    AEntity* SearchCombatTarget(const FVector& Start, const FVector& End, float SearchRadius) const;
    
    bool IsLocking() const { return bIsHardLocking; }
    
protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter;

    UPROPERTY()
    TObjectPtr<AController> OwnerController;

    UPROPERTY()
    AEntity* CurrentTarget;

    UPROPERTY()
    UTimelineComponent* SoftLockTimeline;

    UPROPERTY()
    UCurveFloat* SoftLockCurve;
    
    float SoftLockDistance = 250.f;
    float SoftLockRadius = 100.f;
    bool bIsHardLocking = false;
    float HardLockRadius = 1500.f;
    int32 CombatTargetIndex = 0;

    UPROPERTY()
    TArray<AEntity*> CombatTargets;
    
    UFUNCTION()
    bool PickHardLockTarget();

    UFUNCTION()
    void RotateTowardsHardLockTarget(const AActor* Target, float DeltaTime) const;
    
    UFUNCTION()
    TArray<AEntity*> GetCombatTargets(const float Radius) const;

    UFUNCTION()
    void UpdateSoftLockOn(float Alpha);
};