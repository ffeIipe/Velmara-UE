#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "InteractionSystem/Interfaces/SignalReceiver.h"
#include "MusicZone.generated.h"

class UAudioComponent;
class UBoxComponent;

UCLASS()
class TESISUE_API AMusicZone : public AActor, public ISignalReceiver
{
	GENERATED_BODY()

public:
	AMusicZone();

	virtual void ReceiveSignal_Implementation(bool bActive, AActor* Activator) override;

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StartMusic();

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StopMusic();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TObjectPtr<USoundBase> Music;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bStartOnPlayerOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bPlayOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ToolTip = "Silences the other music zones and keeps them from starting while this one plays"))
	bool bInterruptsOtherMusic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0"))
	float FadeInSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0"))
	float FadeOutSeconds = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0", ToolTip = "0 = plays until stopped"))
	float MaxDurationSeconds = 0.f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void StartIfPlayerInside();
	void FadeOutOver(float Seconds);
	bool IsMusicActive() const;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> Audio;

	TSet<TWeakObjectPtr<AActor>> ActiveSources;
	FTimerHandle DurationTimer;
	bool bPlaying = false;
	bool bHasPlayed = false;
};
