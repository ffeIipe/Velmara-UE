#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionSystem/Interfaces/Interactable.h"
#include "BloodAltar.generated.h"

class UStaticMeshComponent;

/*
 * Altar de Sangre: al interactuar (GA_Interact) abre/cierra el menú de mejoras de arma del personaje
 * (UWeaponUpgradeComponent::ToggleMenu).
 *
 * Para usarlo con BP_Shop/BP_Altar: Class Settings > Parent Class > BloodAltar.
 */
UCLASS()
class TESISUE_API ABloodAltar : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABloodAltar();

	virtual bool ExecuteInteraction_Implementation(AActor* Interactor) override;

	virtual FText GetInteractionText() const override { return InteractionText; }

protected:
	// Bloquea el canal Visibility para que lo detecte el trace de GA_Interact.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> AltarMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Altar")
	FText InteractionText;

	// Para sonido/VFX al usar el altar. bOpened indica si el menú quedó abierto.
	UFUNCTION(BlueprintImplementableEvent, Category = "Altar")
	void OnAltarUsed(AActor* Interactor, bool bOpened);
};
