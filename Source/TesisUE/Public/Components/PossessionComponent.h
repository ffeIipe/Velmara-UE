#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PossessionComponent.generated.h"

struct FPossessionData;
class AEntity;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessionAttemptFailed);
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

	///returns the entity that this one is possessing
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessedEntity() const { return CurrentlyPossessedEntity; }

	///returns the entity that is possessing this one
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessingEntity() const { return PossessedByEntity; }

	///returns true if this entity is being possessed by another entity
    UFUNCTION(BlueprintPure, Category = "Possession")
    bool IsPossessed() const { return PossessedByEntity != nullptr; }

	///returns true if this entity is possessing someone else
    UFUNCTION(BlueprintPure, Category = "Possession")
    bool IsPossessing() const { return CurrentlyPossessedEntity != nullptr; }

    UPROPERTY(BlueprintAssignable)
    FOnPossessionAttemptFailed OnPossessionAttemptFailed;
    
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
    void OnPossessionReleased();
    AEntity* FindPossessionVictim() const;

    UPROPERTY()
    AEntity* OwnerEntity;

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