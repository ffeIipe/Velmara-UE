#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"
#include "TriggerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTagTriggerDelegate, AActor*, Actor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UTriggerComponent : public UBoxComponent
{
	GENERATED_BODY()

public:	
	UTriggerComponent();

	UPROPERTY(BlueprintAssignable)
	FOnTagTriggerDelegate OnActorAccepted;

protected:
	virtual void BeginPlay() override;

	// El actor debe tener AL MENOS UNO de estos tags.
	// Ejemplo: "Character.Player", "Character.Ally"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	FGameplayTagContainer AcceptedTags;

	// Opcional: Tags que IGNORAN el trigger aunque tengan los aceptados
	// Ejemplo: "State.Dead", "State.Ghost"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	FGameplayTagContainer IgnoredTags;

	UFUNCTION()
	virtual void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool HasMatchingGameplayTags(AActor* Actor) const;
};