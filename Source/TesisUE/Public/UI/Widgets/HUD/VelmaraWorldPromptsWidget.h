#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputTypeEnum.h"
#include "VelmaraWorldPromptsWidget.generated.h"

class APlayerController;
class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UVerticalBox;
class UTotemChargeComponent;

enum class EVelmaraWorldPromptKind : uint8
{
	Interact,
	BloodSuck,
	Possess
};

struct FVelmaraWorldPromptEntry
{
	EVelmaraWorldPromptKind Kind = EVelmaraWorldPromptKind::Interact;
	FString KeyLabel;
	FText ActionLabel;

	bool operator==(const FVelmaraWorldPromptEntry& Other) const
	{
		return Kind == Other.Kind
			&& KeyLabel == Other.KeyLabel
			&& ActionLabel.EqualTo(Other.ActionLabel);
	}
};

UCLASS()
class TESISUE_API UVelmaraWorldPromptStackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEntries(const TArray<FVelmaraWorldPromptEntry>& NewEntries);
	void UpdateFade(float DeltaTime);
	void ResetFade();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void RebuildRows();
	UBorder* BuildPromptRow(const FVelmaraWorldPromptEntry& Entry);

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootVBox;

	TArray<FVelmaraWorldPromptEntry> CachedEntries;
	bool bEntriesInitialized = false;
	float FadeAlpha = 0.0f;
};

UCLASS()
class TESISUE_API UVelmaraWorldPromptsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UVelmaraWorldPromptsWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Velmara|Prompts", meta = (ClampMin = "50.0"))
	float InteractRange = 260.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Velmara|Prompts", meta = (ClampMin = "0.02"))
	float ScanInterval = 0.1f;

private:
	void RefreshPromptTargets(APlayerController* PC);
	void UpdateScreenPositions(APlayerController* PC, float DeltaTime);
	FVector GetActorPromptAnchor(const AActor* Actor) const;
	UVelmaraWorldPromptStackWidget* AcquireStackWidget();
	void ReleaseStackWidget(UVelmaraWorldPromptStackWidget* StackWidget);
	void ReleaseAllStacks();
	void HandleInputMethodChanged(ECommonInputType NewInputType);

	UFUNCTION()
	void HandleBloodMarkChanged(AActor* Actor);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVelmaraWorldPromptStackWidget>> PooledStacks;

	TMap<TWeakObjectPtr<AActor>, TObjectPtr<UVelmaraWorldPromptStackWidget>> ActiveStacks;
	TMap<TWeakObjectPtr<AActor>, TObjectPtr<UCanvasPanelSlot>> ActiveSlots;

	FDelegateHandle InputMethodHandle;
	bool bUsingGamepad = false;
	bool bDirtyScan = true;
	float ScanTimer = 0.0f;
};
