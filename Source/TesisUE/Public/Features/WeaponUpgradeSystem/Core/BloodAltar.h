#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionSystem/Interfaces/Interactable.h"
#include "BloodAltar.generated.h"

class UStaticMeshComponent;

UCLASS()
class TESISUE_API ABloodAltar : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABloodAltar();

	virtual bool ExecuteInteraction_Implementation(AActor* Interactor) override;

	virtual FText GetInteractionText() const override { return InteractionText; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AltarMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar")
	FText InteractionText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Altar")
	void OnAltarUsed(AActor* Interactor, bool bOpened);
};
