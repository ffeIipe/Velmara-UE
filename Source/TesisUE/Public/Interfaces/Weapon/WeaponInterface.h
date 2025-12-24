
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

class UBaseDamageType;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponUsed);

UINTERFACE(MinimalAPI, Blueprintable)
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TESISUE_API IWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	FOnWeaponUsed OnWeaponUsed;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetDamageType(TSubclassOf<UBaseDamageType> DamageType);
	
	virtual void EnableVisuals() = 0;
	virtual void DisableVisuals() = 0;
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) = 0;
	virtual void ClearIgnoreActors() = 0;
};
