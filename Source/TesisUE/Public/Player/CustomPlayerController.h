#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/PlayerController.h"
#include "CustomPlayerController.generated.h"

struct FInputActionValue;
enum class EVelmaraAbilityInputID : uint8;
class UInputMappingContext;
class UInputData;
class UUserWidget;
class USoundMix;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPossessionStartedEvent, APawn*, PossessedHost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPossessionEndedEvent, APawn*, ReleasedHost, float, SunGraceDuration);

UCLASS()
class TESISUE_API ACustomPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	ACustomPlayerController();

	virtual void SetupInputComponent() override;
	
	UFUNCTION(BlueprintCallable, Category = "Pause")
	void TogglePauseMenu();

	void SetPauseAudioMixState(bool bPaused);

	virtual void AddYawInput(float Val) override;

	virtual void AddPitchInput(float Val) override;
	
	UPROPERTY(BlueprintReadOnly)
	class APlayerMainHUD* PlayerMainHUD;

	UFUNCTION(BlueprintCallable, Category = "GAS|Possession")
	bool PerformPossession(APawn* NewPawn);

	UFUNCTION(BlueprintCallable, Category = "GAS|Possession")
	bool IsPossessing() const;

	UFUNCTION(BlueprintPure, Category = "GAS|Possession")
	APawn* GetOriginalBody() const { return OriginalBody; }

	UFUNCTION(BlueprintPure, Category = "Possession")
	bool IsOriginalBodyProtected() const;

	UFUNCTION(BlueprintPure, Category = "Possession")
	bool IsPossessedHost(const AActor* Candidate) const;

	UFUNCTION(BlueprintCallable, Category = "Possession")
	void RestoreOriginalBodyEnergy(float Amount);

	UPROPERTY(BlueprintAssignable, Category = "Possession")
	FOnPossessionStartedEvent OnPossessionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Possession")
	FOnPossessionEndedEvent OnPossessionEnded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Possession")
	float PossessionEnergyDrainPerSecond = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Possession")
	float UnpossessStunDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession", meta = (ClampMin = "0.0"))
	float PostPossessionSunGraceDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession", meta = (ClampMin = "0.0"))
	float EmergeOffsetDistance = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession", meta = (ClampMin = "0.0"))
	float EmergeSearchRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession")
	bool bEnableShadowEnergyRegen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession", meta = (ClampMin = "0.0"))
	float ShadowEnergyRegenPerSecond = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession", meta = (ClampMin = "0.05"))
	float ShadowEnergyRegenInterval = 0.25f;
	
	UPROPERTY(BlueprintReadWrite)
	FVector2D InputMovingVector = FVector2D::ZeroVector;
	
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> OptionsMenuWidgetClass_Pause;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	USoundMix* PauseSoundMix;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputData> InputData;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Possession")
	APawn* OriginalBody;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Possession")
	float PossessionTickInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Team")
	FGenericTeamId TeamId;

	UFUNCTION(BlueprintCallable, Category = "AI | Team")
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	UFUNCTION(BlueprintCallable, Category = "AI | Team")
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

private:
	void AbilityInputTagPressed(EVelmaraAbilityInputID InputId);
	
	void AbilityInputTagReleased(EVelmaraAbilityInputID InputId);

	void Input_Move(const FInputActionValue& Value);
	
	void Input_Look(const FInputActionValue& Value);

	void ReleaseHost(APawn* Host);

	void TickPossession();

	void ProtectOriginalBody();

	void RestoreOriginalBody(APawn* ReferenceHost);

	bool FindSafeEmergeLocation(const APawn* ReferenceHost, FVector& OutLocation, FRotator& OutRotation) const;

	void VerifyOriginalBodyEmergeLocation();

	void TickShadowEnergyRegen();

	TWeakObjectPtr<AController> HostAIController;
	TWeakObjectPtr<APawn> LastReleasedHost;
	FGenericTeamId HostTeamId;
	FTimerHandle PossessionTickTimer;
	FTimerHandle ShadowEnergyRegenTimer;
	FTimerHandle EmergeVerifyTimer;
	FVector LastKnownHostLocation = FVector::ZeroVector;
	bool bHasLastKnownHostLocation = false;
	bool bOriginalBodyProtected = false;
	bool bEndingPossession = false;

	UPROPERTY()
	UUserWidget* CurrentPauseMenuInstance;

	UPROPERTY()
	UUserWidget* CurrentOptionsMenuInstance_Pause;

	bool bIsGamePausedExplicitly;
};
