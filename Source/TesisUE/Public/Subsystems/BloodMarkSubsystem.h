#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BloodMarkSubsystem.generated.h"

class UMaterialInterface;
class UMeshComponent;

// Marca de sangre del pulso vampiro: los enemigos marcados (State.BloodMarked) reciben más daño.
UCLASS()
class TESISUE_API UBloodMarkSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Marca a los enemigos vivos del jugador dentro del radio. Volver a marcar renueva la duración.
	UFUNCTION(BlueprintCallable)
	void MarkEnemiesInRange(const FVector& Origin, float Radius, float Duration, UMaterialInterface* OverlayMaterial);

private:
	struct FMark
	{
		double ExpireTime = 0.0;
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PreviousOverlay;
	};

	void UpdateMarks();
	void ClearMark(AActor* Actor, const FMark& Mark) const;
	bool IsEnemyOfPlayer(const AActor* Actor) const;

	TMap<TWeakObjectPtr<AActor>, FMark> Marks;
	FTimerHandle UpdateTimer;
};
