#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DramaticBar.generated.h"

class UProgressBar;

/**
 * Barra con efecto dramatico: fill suave, barra fantasma (retrasada al bajar),
 * flash al recibir dano, shake y pulso con valor bajo.
 * Widget BP hijo: Overlay con "GhostBar" (detras) y "MainBar" (encima). Llamar SetPercent para mover la barra.
 */
UCLASS()
class TESISUE_API UDramaticBar : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Nuevo valor 0..1. El primer llamado (o bInstant) salta sin animar. */
	UFUNCTION(BlueprintCallable, Category = "DramaticBar")
	void SetPercent(float NewPercent, bool bInstant = false);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> MainBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> GhostBar;

	/** Velocidad del fill principal (mas alto = mas rapido). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float FillSpeed = 10.f;

	/** Segundos que la barra fantasma espera antes de bajar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float GhostDelay = 0.15f;

	/** Suavizado de la estela (mas bajo = mas lenta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar")
	float GhostSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Flash")
	FLinearColor FlashColor = FLinearColor::White;

	/** Segundos que dura el flash al perder valor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Flash")
	float FlashDuration = 0.25f;

	/** Pixeles de shake al perder valor. 0 = sin shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Shake")
	float ShakeIntensity = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Shake")
	float ShakeDuration = 0.25f;

	/** Por debajo de este porcentaje la barra pulsa. 0 = sin pulso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Pulse")
	float LowThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DramaticBar|Pulse")
	float PulseSpeed = 6.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float Target = 1.f;
	float Display = 1.f;
	float Ghost = 1.f;
	float GhostHold = 0.f;
	float FlashLeft = 0.f;
	float ShakeLeft = 0.f;
	float PulseTime = 0.f;
	bool bInitialized = false;
	FLinearColor BaseColor = FLinearColor::White;
};
