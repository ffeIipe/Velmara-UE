#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <Player/CharacterStates.h>
#include "SpectralObjectComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpectralObjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpectralObjectComponent();

	UFUNCTION(BlueprintCallable)
	virtual void SetSpectralVisibility(bool bIsVisible);

	UPROPERTY(EditDefaultsOnly, Category = "Visibility")
	ECharacterForm VisibleTo;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Visibility")
	TArray<UStaticMeshComponent*> Doors;

private:
	TArray<UMaterialInstanceDynamic*> SpectralMaterial;
};
