#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectralTrapComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpectralTrapComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpectralTrapComponent();

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SphereCollider;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Damage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FVector LaunchStrenght;

	AController* Instigator;
};
