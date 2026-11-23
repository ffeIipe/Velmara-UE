#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Sword.h"
#include "DualSword.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDualSwordHand : uint8
{
	Both,
	Right,
	Left
};

UCLASS()
class TESISUE_API ADualSword : public ASword
{
	GENERATED_BODY()

public:
	ADualSword();

	virtual void AttachMeshToSocket(USceneComponent* InParent, FName InSocketName = NAME_None) override;
	virtual void SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled) override;
	virtual void EnableVisuals() override;
	virtual void DisableVisuals() override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetActiveHand(EDualSwordHand NewHand);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dual Sword")
	FName OffHandSocketName = "LeftHandSword";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dual Sword")
	FName OffHandHolsterSocketName = "LeftClavicleSocket";

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dual Sword")
	TObjectPtr<UStaticMeshComponent> OffHandMesh;

	UPROPERTY(VisibleAnywhere, Category = "Dual Sword")
	TObjectPtr<UBoxComponent> OffHandDamageBox;

	UPROPERTY(VisibleAnywhere, Category = "Dual Sword")
	TObjectPtr<USceneComponent> OffHandTraceStart;

	UPROPERTY(VisibleAnywhere, Category = "Dual Sword")
	TObjectPtr<USceneComponent> OffHandTraceEnd;

	UFUNCTION()
	void OnOffHandOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void ApplyCollision();

	ECollisionEnabled::Type CollisionState = ECollisionEnabled::NoCollision;
	EDualSwordHand ActiveHand = EDualSwordHand::Both;
};
