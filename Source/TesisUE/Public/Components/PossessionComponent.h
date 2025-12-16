// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PossessionComponent.generated.h"

class AEntity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPossessed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReleasePossession);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReleaseAndExecute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCannotPossess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCannotRelease);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UPossessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPossessionComponent();

	UFUNCTION(BlueprintCallable)
	AEntity* GetPossessionOwner() { return PossessionOwner; };
	
	UFUNCTION(BlueprintCallable)
	AEntity* GetEntityPossessed() { return EntityPossessed; };

	void Possess();

	bool CanBePossessed() { return bCanBePossessed; };

	FOnPossessed OnPossessed;
	FOnReleasePossession OnReleasePossession;
	FOnReleaseAndExecute OnReleaseAndExecute;
	FOnCannotPossess OnCannotPossess;
	FOnCannotRelease OnCannotRelease;

	void ReceivePossession(AEntity* NewOwner, AEntity* TargetEnemy, float OwnerEnergy);

	void ReleasePossession();

	void ReleaseAndExecute();

private:
	void BeginPlay() override;


	AEntity* GetPossessionVictim();

	/*
	* ----------Stats----------
	*/
	UPROPERTY(EditAnywhere, Category = "Stats");
	float PossessDistance;
	
	UPROPERTY(EditAnywhere, Category = "Stats");
	float PossessRadius;
	
	UPROPERTY(EditAnywhere, Category = "Stats");
	bool bCanBePossessed = true;

	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float ReleaseEnergyTax = 3.f;

	UPROPERTY(EditAnywhere, Category = "Energy| Energy Tax");
	float ReleaseAndExecuteEnergyTax = 3.f;
	
	AEntity* PossessionOwner;

	AEntity* EntityPossessed;
	
	AEntity* EntityOwner;

	APlayerController* PlayerControllerRef;
};
