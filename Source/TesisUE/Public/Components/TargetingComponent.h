#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/CombatTargetInterface.h"
#include "TargetingComponent.generated.h"

struct FTargetingData;
class UTimelineComponent;
class ICharacterMovementProvider;
class IControllerProvider;
class ICameraProvider;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHardLockToggled);

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

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void ToggleHardLock();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void ChangeHardLockTarget();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void PerformSoftLock();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    TScriptInterface<ICombatTargetInterface> GetCurrentTarget() { return CurrentTarget; }

    UFUNCTION()
    void RemoveCombatTarget();

    UFUNCTION(BlueprintCallable, Category = "Targeting")
    TScriptInterface<ICombatTargetInterface> SearchCombatTarget(const FVector& Start, const FVector& End, float SearchRadius) const;
    
    bool IsLocking() const { return bIsHardLocking; }
    
protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TScriptInterface<IControllerProvider> ControllerProvider;

    UPROPERTY()
    TScriptInterface<ICameraProvider> CameraProvider;

    UPROPERTY()
    TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;

    UPROPERTY()
    TScriptInterface<ICombatTargetInterface> CurrentTarget;

    UPROPERTY()
    UTimelineComponent* SoftLockTimeline;

    UPROPERTY()
    UCurveFloat* SoftLockCurve;
    
    float SoftLockDistance = 250.f;
    float SoftLockRadius = 100.f;
    bool bIsHardLocking = false;
    float HardLockRadius = 1500.f;
    int32 CombatTargetIndex = 0;
    TArray<TScriptInterface<ICombatTargetInterface>> CombatTargets;

    UFUNCTION()
    bool PickHardLockTarget();

    UFUNCTION()
    void ValidateCurrentTarget();
    
    UFUNCTION()
    void RotateTowardsHardLockTarget(const TScriptInterface<ICombatTargetInterface>& Target, float DeltaTime) const;
    
    UFUNCTION()
    TArray<TScriptInterface<ICombatTargetInterface>> GetCombatTargets(const float Radius) const;

    UFUNCTION()
    void UpdateSoftLockOn(float Alpha);
};