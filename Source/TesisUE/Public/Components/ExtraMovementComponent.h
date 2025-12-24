#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExtraMovementComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnDodgeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDodgeSaved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoubleJumpStarted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UExtraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UExtraMovementComponent();
	
	UFUNCTION()
	void ResetDodge() { bIsSaveDodge = false; }

	UFUNCTION(BlueprintCallable)
	void DodgeSaveEvent();

	UFUNCTION(BlueprintCallable)
	bool IsMovingBackwards() const;

	bool IsMoving() const { return bIsMoving; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsSaveDodge = false;

	FOnDodgeStarted OnDodgeStarted;
	FOnDodgeSaved OnDodgeSaved;
	FOnDoubleJumpStarted OnDoubleJumpStarted;
	
private:
	virtual void BeginPlay() override;

	FName DodgeAnimBasedOnInput(UAnimMontage* DodgeMontage) const;

	bool bIsMoving = false;
	FVector2D CurrentMoveVector = FVector2D::ZeroVector;
};
