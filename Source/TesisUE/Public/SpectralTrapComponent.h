#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectralTrapComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESISUE_API USpectralTrapComponent : public UActorComponent
{
	GENERATED_BODY()

	//DECLARE_MULTICAST_DELEGATE(FOnSignature);

public:	
	USpectralTrapComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USphereComponent* SphereCollider;

	void FinishDamaging();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Stats")
	float DamageInterval = 1.f;

	FTimerHandle ContinuousDamageTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FVector LaunchStrenght;

	AController* Instigator;

	UPROPERTY(VisibleAnywhere)
	class APlayerMain* OverlappingPlayer;

	void DealContinuousDamage();
};
