#pragma once

#include "CoreMinimal.h"
#include "DataAssets/Items/Weapons/PistolData.h"
#include "Features/InventorySystem/Items/Weapon.h"
#include "Pistol.generated.h"

class UPistolData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFire);

UCLASS()
class TESISUE_API APistol : public AWeapon
{
	GENERATED_BODY()
public:
	virtual void AttachMeshToSocket(USceneComponent* InParent, FName SocketInName) override;

	UFUNCTION(BlueprintPure, Category = "Weapon Properties")
	UPistolData* GetPistolData() const { return PistolData; }
	
protected:
	APistol();
	virtual void BeginPlay() override;

	virtual void PrimaryShoot();

	virtual void SecondaryShoot();

	FOnFire OnFire;
    
private:
	void Fire();

	void EnableFire();

	void Reload();
    
	void FinishReload();

	void SetTimer(FTimerHandle TimerHandle, float Time, void (APistol::*InTimerMethod)());

	UFUNCTION()
	void PlayEffects();

	UPROPERTY()
	int32 CurrentAmmo;

	UPROPERTY()
	bool bIsFireEnabled = false;
    
	UPROPERTY()
	bool bIsReloading = false;

	FTimerHandle TimerHandle_Reload;
	FTimerHandle TimerHandle_BetweenShots;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UPistolData* PistolData; 
};