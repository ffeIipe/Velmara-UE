#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PossessionComponent.generated.h"

class IWeaponProvider;
class IAnimatorProvider;
class ICameraProvider;
class IAttributeProvider;
class ICharacterStateProvider;
class IOwnerUtilsInterface;
struct FPossessionData;
class AEntity;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessionAttemptFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessionAttemptSucceed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessionReleased);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessorEjected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessorExecutedMeAndEjected);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TESISUE_API UPossessionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPossessionComponent();
    void InitializeValues(const FPossessionData& PossessionData);
    
    void AttemptPossession();
    void ReleasePossession();
    void EjectPossessor();
    void EjectAndExecute();

	///returns the entity that possesses
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessedEntity() const { return CurrentlyPossessedEntity; }

	///returns the entity that possesses this one
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessor() const { return PossessedByEntity; }

	///returns true if this entity is being possessed by another entity
    UFUNCTION(BlueprintPure, Category = "Possession")
    bool IsPossessed() const { return PossessedByEntity != nullptr; }

	///returns true if this entity is possessing someone else
    UFUNCTION(BlueprintPure, Category = "Possession")
    bool IsPossessing() const { return CurrentlyPossessedEntity != nullptr; }

    UPROPERTY(BlueprintAssignable)
    FOnPossessionAttemptFailed OnPossessionAttemptFailed;

    UPROPERTY(BlueprintAssignable)
    FOnPossessionAttemptSucceed OnPossessionAttemptSucceed;

    UPROPERTY(BlueprintAssignable)
    FOnPossessionReleased OnPossessionReleased;
    
    UPROPERTY(BlueprintAssignable)
    FOnPossessed OnPossessed;

    UPROPERTY(BlueprintAssignable)
    FOnPossessorEjected OnPossessorEjected;
    
    UPROPERTY(BlueprintAssignable)
    FOnPossessorExecutedMeAndEjected OnPossessorExecutedMeAndEjected;

protected:
    virtual void BeginPlay() override;

private:
    void OnPossessionReceived(AEntity* NewPossessor);
    void ReleasingPossession();
    AEntity* FindPossessionVictim() const;

    UPROPERTY()
    TScriptInterface<IOwnerUtilsInterface> OwnerUtils;
    TScriptInterface<ICharacterStateProvider> CharacterStateProvider;
    TScriptInterface<IAttributeProvider> AttributeProvider;
    TScriptInterface<ICameraProvider> CameraProvider;
    TScriptInterface<IAnimatorProvider> AnimatorProvider;
    TScriptInterface<IWeaponProvider> WeaponProvider;

    UPROPERTY()
    APlayerController* PlayerController;

    UPROPERTY()
    AEntity* CurrentlyPossessedEntity = nullptr;

    UPROPERTY()
    AEntity* PossessedByEntity = nullptr;

    float PossessDistance = 1500.f;
    float PossessRadius = 50.f;
    float ReleaseAndExecuteEnergyTax = 25.f;
};