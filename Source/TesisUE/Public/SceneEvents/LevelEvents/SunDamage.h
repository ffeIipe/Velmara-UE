#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SunDamage.generated.h"

class ADirectionalLight;

UCLASS()
class TESISUE_API ASunDamage : public AActor
{
	GENERATED_BODY()

public:
	ASunDamage();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Properties")
	bool bIsDebugEnabled = true;

	UPROPERTY(EditAnywhere, Category = "Properties")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType = EDrawDebugTrace::None;

	UPROPERTY(EditAnywhere, Category = "Properties")
	TEnumAsByte<ECollisionChannel> WallCollisionChannel = ECC_GameTraceChannel4;
	
	UPROPERTY(EditAnywhere, Category = "Properties")
	float DamageInterval = .15f;

	UPROPERTY(EditAnywhere, Category = "Properties")
	float MaxDamage = 1.f;

	UPROPERTY(EditAnywhere, Category = "Properties")
	float MinDamage = 1.f;

	UPROPERTY(EditAnywhere, Category = "Properties")
	UCurveFloat* DamageCurve = nullptr;
	
	UPROPERTY()
	ADirectionalLight* DirectionalLight = nullptr;
	
	UPROPERTY()
	AActor* PlayerActor = nullptr;

	float TimeOnTarget = 0.f;
	
	float Damage = 0.f;
	
	FVector SunLocation = FVector::ZeroVector;
};
