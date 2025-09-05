#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

struct FInputActionValue;
class ICharacterMovementProvider;
class IAnimatorProvider;
class IWeaponInterface;
class IWeaponProvider;
class IControllerProvider;
class ICameraProvider;
class ICombatTargetInterface;
class ICharacterStateProvider;
class UCharacterStateComponent;
class IOwnerUtilsInterface;
struct FCombatData;
class UEntityData;
class UTimelineComponent;
class UCurveFloat;
class IFormInterface;
class UExtraMovementComponent;
class ASword;
class AEntity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackEnd);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // --- Delegates ---
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallHitSignature, const FHitResult&, HitResult);
    FOnWallHitSignature OnWallHit;
    UPROPERTY(BlueprintAssignable)
    FOnAttackEnd OnAttackEnd;

    // --- Component References ---
    UPROPERTY()
    AController* OwnerController;
    UPROPERTY()
    TScriptInterface<IControllerProvider> ControllerProvider;
    UPROPERTY()
    TScriptInterface<IWeaponProvider> WeaponProvider;
    UPROPERTY()
    TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
    UPROPERTY()
    TScriptInterface<IOwnerUtilsInterface> OwnerUtils;
    UPROPERTY()
    TScriptInterface<ICharacterMovementProvider> CharacterMovementProvider;
    UPROPERTY()
    TScriptInterface<ICombatTargetInterface> CombatTarget;
    UPROPERTY()
    TScriptInterface<ICameraProvider> CameraProvider;
    UPROPERTY()
    TScriptInterface<IAnimatorProvider> AnimatorProvider;
    
    // --- Finisher Locations ---
    UPROPERTY(EditAnywhere, Category = "Attack | Finisher")
    USceneComponent* CameraFinisherLocation;
    
    // --- Public Functions ---
    UCombatComponent();
    void InitializeValues(const FCombatData& CombatData);
    UFUNCTION(BlueprintCallable, Category = "Attack")
    void ResetState();
    UFUNCTION()
    void RemoveCombatTarget() {CombatTarget = nullptr;}
    UFUNCTION()
    bool IsBlocking() const;
    UFUNCTION()
    void ReceiveBlock() const;
    
    // --- Input Functions ---
    void Input_Attack();
    void Input_HeavyAttack();
    void Input_Launch();
    void Input_Block(const FInputActionValue& Value);
    void ChangeHardLockTarget();
    void ToggleHardLock();
    UFUNCTION() // ufunction bc it's called by an event that executes when you cant possess
    void Input_Execute();
    
    // --- Attack Events ---
    UFUNCTION()
    void AttackEvent();
    UFUNCTION()
    void HeavyAttackEvent();
    UFUNCTION()
    void Execute();
    
    // --- Combat Utility Functions ---
    void StartLaunchingUp();
    UFUNCTION()
    TScriptInterface<ICombatTargetInterface> SearchCombatTarget(const FVector& Start, const FVector& End);
    
    // --- Buffer Distance ---
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
    
    // --- Attack State Flags ---
    UPROPERTY(VisibleAnywhere, Category = "Attack | LightAttack")
    bool bIsSaveLightAttack;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack | JumpAttack")
    bool bIsLaunched = false;
    UPROPERTY(BlueprintReadOnly, Category = "Attack | HardLock")
    bool bIsHardLocking = false;
    
    // --- Launch Character Properties ---
    UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
    FVector CurrentLocationLaunch;
    UPROPERTY(BlueprintReadOnly, Category = "LaunchCharacter")
    FVector UpVectorLaunch;
    
    // --- Buffer Attack ---
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    float BufferMultiplier = 1.f;

protected:
    // --- Lifecycle Events ---
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    // --- Attack Logic Functions ---
    UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
    void LightAttack(int AttackIndex);
    UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack")
    void ResetLightAttackStats();
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void JumpAttack(int AttackIndex);
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void ResetJumpAttackStats();
    UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
    void HeavyAttack(int AttackIndex);
    UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack")
    void ResetHeavyAttackStats();
    UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
    void PerformComboStarter(int AttackIndex);
    UFUNCTION(BlueprintCallable, Category = "Attack | ComboAttack")
    void PerformComboExtender(int AttackIndex);
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void LaunchCharacterUp();
    UFUNCTION(BlueprintCallable, Category = "Attack | JumpAttack")
    void Crasher();
    
    // --- Targeting ---
    UFUNCTION()
    bool PickHardLockTarget();
    UFUNCTION()
    bool IsValidAndAlive(const TScriptInterface<ICombatTargetInterface>& TargetToCheck);
    UFUNCTION()
    TArray<TScriptInterface<ICombatTargetInterface>> GetCombatTargets(const float Radius) const;
    UFUNCTION()
    void RotateTowardsHardLockTarget(const TScriptInterface<ICombatTargetInterface>& HardLockTarget, float DeltaTime) const;
    
    // --- Blocking ---
    UFUNCTION()
    void Block() const;
    
    UFUNCTION(BlueprintCallable, Category = "Block")
    void ReleaseBlock() const;
    

    // --- Soft Lock Targeting ---
    UFUNCTION(BlueprintCallable, Category = "SoftLock")
    void SoftLockOn();
    UFUNCTION(BlueprintCallable, Category = "SoftLock")
    void ValidateWall();
    UFUNCTION(BlueprintCallable, Category = "SoftLock")
    void RotationToTarget();
    UFUNCTION()
    void UpdateSoftLockOn(float Alpha);
    
    // --- Launch Character Timeline ---
    UFUNCTION()
    void UpdateLaunchCharacterUp(float Alpha);
    
    // --- Buffer Attack ---
    UFUNCTION()
    void UpdateAttackBuffer(float Alpha) const;
    void UpdateBuffer(float Alpha, float BufferDistance) const;
    
    // --- Animation Montages ---
    UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
    TArray<UAnimMontage*> ComboStarterAttack;
    UPROPERTY(EditDefaultsOnly, Category = "Montages | ComboAttack")
    TArray<UAnimMontage*> ComboExtenderAttack;
    UPROPERTY(EditAnywhere)
    UAnimMontage* BlockMontage;
    UPROPERTY(EditAnywhere)
    UAnimMontage* FinisherMontage;
    UPROPERTY(EditAnywhere)
    UAnimMontage* CrasherMontage;
    UPROPERTY(EditAnywhere)
    UAnimMontage* LaunchMontage;
    UPROPERTY(EditAnywhere)
    UAnimMontage* HitReactMontage;
    
private:
    // --- Internal State Variables ---
    int LightAttackIndex = 0;
    int JumpAttackIndex = 0;
    int HeavyAttackIndex = 0;
    int ComboExtenderIndex = 0;
    bool bIsSaveHeavyAttack;
    int CombatTargetIndex = 0;
    float HardLockRadius = 1500.f;
    UPROPERTY()
    TArray<TScriptInterface<ICombatTargetInterface>> CombatTargets;
    
    // --- Internal Utility Functions ---
    bool CanAttack() const;
    bool CheckDistance(const FVector& Origin, const FVector& Target, float DistanceToCheck);
    
    // --- Save Attack Events ---
    UFUNCTION(BlueprintCallable, Category = "Attack | LightAttack", meta = (AllowPrivateAccess = "true"))
    void SaveLightAttackEvent();
    UFUNCTION(BlueprintCallable, Category = "Attack | HeavyAttack", meta = (AllowPrivateAccess = "true"))
    void SaveHeavyAttackEvent();
    UFUNCTION(BlueprintCallable, Category = "Attack | SaveAttack", meta = (AllowPrivateAccess = "true"))
    void ResetAttackSave();
    
    // --- Component References ---
    TScriptInterface<IWeaponInterface> GetCurrentWeapon();
    
    // --- Timelines ---
    UPROPERTY()
    UTimelineComponent* SoftLockTimeline;
    UPROPERTY()
    UTimelineComponent* LaunchCharacterTimeline;
    UPROPERTY()
    UTimelineComponent* BufferAttackTimeline;
    UPROPERTY()
    UTimelineComponent* BufferBackwardsTimeline;

    // --- Stats Assigned By Data Asset ---
    float SoftLockDistance;
    float SoftLockRadius;
    float TrackTargetRadius;
    float BufferAttackDistance;
    UPROPERTY()
    UCurveFloat* SoftLockCurve;
    UPROPERTY()
    UCurveFloat* BufferCurve;
    
    //TODO: Add trough DataAsset
    // UPROPERTY()
    // UCurveFloat* BufferLaunchUp;
    // UPROPERTY()
    // UCurveFloat* BufferBackwardsCurve;
};