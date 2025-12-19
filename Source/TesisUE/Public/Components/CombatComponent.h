#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class IWeaponInterface;
struct FCombatData;
class UAttributeComponent;
class UCharacterStateComponent;
class UTimelineComponent;
class ICombatTargetInterface;
enum class ECharacterModeStates : uint8;
struct FInputActionValue;

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanPerformActionSignature, FGameplayTag);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    FCanPerformActionSignature CanPerformActionDelegate;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionPerformedSignature, FGameplayTag);
    FOnActionPerformedSignature OnActionPerformed;

    //TODO: Delete unused Delegates
    
    // === Delegates ===
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEnd);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLightAttack);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeavyAttack);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveLightAttack);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveHeavyAttack);
    
    FOnWallHitSignature OnWallHit;
    UPROPERTY(BlueprintAssignable)
    FOnAttackEnd OnResetState;
    FOnLightAttack OnLightAttack;
    FOnHeavyAttack OnHeavyAttack;
    FOnSaveLightAttack OnSaveLightAttack;
    FOnSaveHeavyAttack OnSaveHeavyAttack;

    UFUNCTION()
    void HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon);
    
    // === Finisher Locations ===
    UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
    USceneComponent* CameraFinisherLocation;

    // === Public Functions ===
    UCombatComponent();

    void InitializeValues(const FCombatData& CombatData);

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ResetState();

    UFUNCTION()
    bool IsBlocking() const;

    UFUNCTION(BlueprintCallable)
    bool IsInAir() const;
    
    bool CanAttack(FGameplayTag ActionTag) const;
    
    void PerformBlock(bool bIsTriggered, UAnimMontage* BlockMontage) const;

    UFUNCTION()
    void ReceiveBlock(UAnimMontage* BlockMontage) const;

    UFUNCTION(BlueprintCallable)
    bool PerformLaunch(const TScriptInterface<ICombatTargetInterface>& TargetToCheck, float DistanceToCheck = 200.f, UAnimMontage* LaunchMontage = nullptr);
     
    // === Internal Utility Functions ===
    bool CheckDistance(const TScriptInterface<ICombatTargetInterface>& TargetToCheck, float DistanceToCheck);
    
    
    // === Attack Events ===
    UFUNCTION(BlueprintCallable)
    void PerformExecute(const TScriptInterface<ICombatTargetInterface>& Target, UAnimMontage* FinisherMontage) const;
    
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
    // === Lifecycle Events ===
    virtual void BeginPlay() override;
    
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void LaunchCharacterUp(TScriptInterface<ICombatTargetInterface> Target);
    
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void PerformCrasher();
    
    // === Blocking ===
    UFUNCTION()
    void Block(UAnimMontage* BlockMontage) const;
    
    UFUNCTION(BlueprintCallable, Category = "Block")
    void ReleaseBlock(UAnimMontage* BlockMontage) const;
    
    // === Soft Lock Targeting ===
    //UFUNCTION(BlueprintCallable, Category = "SoftLock")
    //void ValidateWall();
    
    // === Launch Character Timeline ===
    UFUNCTION()
    void UpdateLaunchCharacterUp(float Alpha);

    // === Buffer Attack ===
    UFUNCTION()
    void UpdateAttackBuffer(float Alpha) const;
    void UpdateBuffer(float Alpha, float BufferDistance) const;
    
private:
    // === Internal State Variables ===
    UPROPERTY()
    TArray<TScriptInterface<ICombatTargetInterface>> CombatTargets;
    
    // === Save Attack Events ===
    UFUNCTION(BlueprintCallable)
    void SaveLightAttackEvent();
    
    UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack", meta = (AllowPrivateAccess = "true"))
    void SaveHeavyAttackEvent();
    
    UFUNCTION(BlueprintCallable, Category = "Attack | SaveAttack", meta = (AllowPrivateAccess = "true"))
    void ResetAttackSave();
    
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
    TObjectPtr<UCharacterStateComponent> StateComponent;

    UPROPERTY()
    TObjectPtr<UAttributeComponent> AttributeComponent;

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