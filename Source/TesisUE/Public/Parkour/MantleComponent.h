#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MantleComponent.generated.h"

USTRUCT(BlueprintType)
struct FLedgeDetectionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bCanClimb = false;

	UPROPERTY(BlueprintReadOnly)
	FVector WallLocation;

	UPROPERTY(BlueprintReadOnly)
	FVector LedgeLocation;

	UPROPERTY(BlueprintReadOnly)
	FVector LandingLocation;
    
	UPROPERTY(BlueprintReadOnly)
	FVector WallNormal;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API UMantleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMantleComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ClimbingTraceDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float ClimbingHeadHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Debug")
	bool bDrawDebugTraces = false;

	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool DetectLedge(FLedgeDetectionResult& OutResult) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ACharacter* OwnerCharacter;
};