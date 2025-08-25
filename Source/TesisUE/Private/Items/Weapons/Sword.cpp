#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"
#include "Engine/DamageEvents.h"
#include "Player/PlayerMain.h"
#include "Components/CharacterStateComponent.h"
#include <NiagaraFunctionLibrary.h>

#include "Subsystems/EffectsManager.h"

ASword::ASword()
{
	BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box"));
	WeaponBox->SetupAttachment(GetRootComponent());
	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WeaponBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(GetRootComponent());

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(GetRootComponent());
}

void ASword::BeginPlay()
{
	Super::BeginPlay();

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &ASword::OnBoxOverlap);
}

void ASword::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	Super::Equip(InParent, InSocketName, NewOwner, NewInstigator);

	AttachMeshToSocket(InParent, InSocketName);
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	ItemState = EItemState::EIS_Equipped;

	EnableSwordState(true);

	if (AEntity* EntityRef = Cast<AEntity>(NewOwner))
	{
		EntityRef->GetCharacterStateComponent()->SetCharacterState(ECharacterStates::ECS_EquippedSword);
	}
}

void ASword::Unequip()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	EnableSwordState(false);
	//la anim la pongo en el propio player al cambiar de arma
	AttachMeshToSocket(OwnerCharacter->GetMesh(), FName("BackSocket"));
}

void ASword::AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName)
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

void ASword::EnableSwordState(const bool bEnable)
{
	if (CharacterStateComponent)
	{
		const ECharacterStates NewState = bEnable ? ECharacterStates::ECS_EquippedSword : ECharacterStates::ECS_Unequipped;
		CharacterStateComponent->SetCharacterState(NewState);
	}
}

UPrimitiveComponent* ASword::GetCollisionComponent()
{
	return WeaponBox;
}

void ASword::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void ASword::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

void ASword::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	IgnoreActors.Add(this);
	IgnoreActors.Add(GetOwner());

	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(25.f, 25.f, 25.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (const IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
			{
				FDamageEvent DamageEvent(DamageTypeClass);
				HitInterface->Execute_GetHit(HitActor, Cast<AEntity>(GetOwner()), Hit.ImpactPoint, DamageEvent, Damage);

				if (HitInterface->Execute_IsLaunchable(HitActor))
				{
					UGameplayStatics::ApplyDamage(
						HitActor,
						CalculateDamage(),
						GetInstigator()->GetController(),
						GetOwner(),
						DamageTypeClass
					);

					//fx when the hit is true
					if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
					{
						EffectsManager->HitStop(EHitStopPreset::EHSP_SwordHit);
						EffectsManager->CameraShake(ECameraShakePreset::ECSP_SwordHit, Hit.ImpactPoint);
					}

					if (HitEffect)
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							GetWorld(),
							HitEffect,
							Hit.ImpactPoint,
							Hit.ImpactNormal.Rotation(),
							FVector(1.f),
							true
						);
					}
					
					IgnoreActors.Add(HitActor);
				}
				else //else if it is not hittable
				{
					if (SparksEffect)
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							GetWorld(),
							SparksEffect,
							Hit.ImpactPoint,
							Hit.ImpactNormal.Rotation(),
							FVector(1.f),
							true
						);
					}

					if (ShieldImpactSFX)
					{
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Hit.ImpactPoint);
					}
				}
			}
		}
	}
}

float ASword::CalculateDamage()
{
	if (FMath::FRandRange(0.f, 1.f) <= CriticalChance)
	{
		return Damage * CriticalDamageMultiplier;
	}
	return Damage;
}

void ASword::HitStop(const float Duration, const float TimeScale)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(TimeScale);

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, this, &ASword::ResetTimeDilation, Duration, false);
	}
}

void ASword::ResetTimeDilation()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(1.0f);
	}
}