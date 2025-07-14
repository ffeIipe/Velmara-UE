#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PossessionComponent.generated.h"

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

    void AttemptPossession();
    void ReleasePossession();
    void EjectPossessor();
    void EjectAndExecute();

	//retorna la entidad que este Entity esta poseyendo
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessedEntity() const { return CurrentlyPossessedEntity; }

	//retorna la entidad que posee a este Entity
    UFUNCTION(BlueprintPure, Category = "Possession")
    AEntity* GetPossessingEntity() const { return PossessedByEntity; }

	//retorna true si esta entidad esta siendo poseida por otra entidad
    UFUNCTION(BlueprintPure, Category = "Possession")
    bool IsPossessed() const { return PossessedByEntity != nullptr; }

	//retorna true si esta entidad esta poseyendo a otra entidad
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
    AEntity* FindPossessionVictim();

    UPROPERTY(EditDefaultsOnly, Category = "Possession")
    float PossessDistance = 1500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Possession")
    float PossessRadius = 50.f;

    UPROPERTY(EditDefaultsOnly, Category = "Possession")
    float ReleaseAndExecuteEnergyTax = 25.f;

    UPROPERTY()
    AEntity* OwnerEntity;

    UPROPERTY()
    APlayerController* PlayerController;

    UPROPERTY()
    AEntity* CurrentlyPossessedEntity = nullptr;

    UPROPERTY()
    AEntity* PossessedByEntity = nullptr;
};