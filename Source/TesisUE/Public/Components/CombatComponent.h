#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AEntity;
struct FCombatData;
class UTimelineComponent;

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanPerformActionSignature, FGameplayTag);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    FCanPerformActionSignature CanPerformActionDelegate;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionPerformedSignature, FGameplayTag);
    FOnActionPerformedSignature OnActionPerformed;

    UFUNCTION()
    void HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon);
    
    // === Finisher Locations ===
    UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
    USceneComponent* CameraFinisherLocation;

    // === Public Functions ===
    UCombatComponent();

    void InitializeValues(const FCombatData& CombatData);
     
    // === Internal Utility Functions ===
    bool CheckDistance(const AEntity* TargetToCheck, float DistanceToCheck) const;
    
    // === Combat Utility Functions ===
    void StartLaunchingUp();
   
    // === Buffer Distance ===
    UFUNCTION(BlueprintCallable)
    void StartAttackBufferEvent();
    
    UFUNCTION(BlueprintCallable)
    void StopAttackBufferEvent();

    UFUNCTION()
    void StartBufferBackwards();
    UFUNCTION()
    void StopBufferBackwards();
    UFUNCTION()
    void UpdateBufferBackwards(float Alpha);
    
    // === Attack State Flags ===
    UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
    bool bIsSaveLightAttack;
    UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
    bool bIsSaveHeavyAttack;;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack | JumpAttack")
    bool bIsLaunched = false;
    
    // === Launch Character Properties ===
    UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
    FVector CurrentLocationLaunch;
    UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
    FVector UpVectorLaunch;
    
    // === Buffer Attack ===
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    float BufferMultiplier = 1.f;

protected:
    virtual void BeginPlay() override;

    // === Buffer Attack ===
    UFUNCTION()
    void UpdateAttackBuffer(float Alpha) const;
    void UpdateBuffer(float Alpha, float BufferDistance) const;
    
private:
    // === Internal State Variables ===
    UPROPERTY()
    TArray<AEntity*> CombatTargets;
    
    // === Timelines ===
    UPROPERTY()
    UTimelineComponent* LaunchCharacterTimeline;
    UPROPERTY()
    UTimelineComponent* BufferAttackTimeline;
    UPROPERTY()
    UTimelineComponent* BufferBackwardsTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Combat | Config")
    FGameplayTagContainer BlockAttackTags;
    
    UPROPERTY()
    TObjectPtr<ACharacter> OwnerCharacter;
    
    // === Stats Assigned By Data Asset ===
    float BufferAttackDistance;
    
    UPROPERTY()
    UCurveFloat* BufferCurve;
    UPROPERTY()
    UCurveFloat* LaunchCharacterUpCurve;
    UPROPERTY()
    UCurveFloat* BufferBackwardsCurve;
};