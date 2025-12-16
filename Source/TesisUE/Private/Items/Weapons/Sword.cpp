// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"

#include "Player/PlayerMain.h"
#include "Interfaces/CharacterState.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CombatComponent.h"
#include <NiagaraFunctionLibrary.h>
#include <Kismet/KismetMathLibrary.h>

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

	CharacterStateInterface = Cast<ICharacterState>(NewOwner);
	if (CharacterStateInterface)
	{
		CharacterStateComponent = CharacterStateInterface->Execute_GetCharacterStateComponent(NewOwner);
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

void ASword::EnableSwordState(bool bEnable)
{
	if (CharacterStateComponent)
	{
		ECharacterStates NewState = bEnable ? ECharacterStates::ECS_EquippedSword : ECharacterStates::ECS_Unequipped;
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
			if (IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
			{
				float TempDamage = CalculateDamage();

				/*GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Cyan, FString::SanitizeFloat(Damage));*/
				HitInterface->Execute_GetHit(HitActor, GetOwner(), Hit.ImpactPoint, UDamageType::StaticClass(), Damage);

				if (HitInterface->Execute_IsLaunchable(HitActor, Cast<ACharacter>(Owner)))
				{
					TSubclassOf<UDamageType> FinalDamageType = DamageTypeClass ? DamageTypeClass : TSubclassOf<UDamageType>(UDamageType::StaticClass());

					UGameplayStatics::ApplyDamage(
						HitActor,
						Damage,
						GetInstigator()->GetController(),
						GetOwner(),
						FinalDamageType
					);

					//fx when the hit is true
					CameraShake();
					HitStop(.0005f, .01f);

					IgnoreActors.Add(HitActor);
				}
				else //else si no es hitteable (tengo que cambiar nombre de la funcion)
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

					//APlayerMain* PlayerRef = Cast<APlayerMain>(GetOwner());
					//PlayerRef->CombatComponent->HitReactJumpToSection(FName("ReactToShield"));

					if (ShieldImpactSFX)
					{
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Hit.ImpactPoint);
					}
				}
				
				
			}
			//else
			//{
			//	OnWallHit.Broadcast(Hit);
			//}
		}
	}
}

float ASword::CalculateDamage()
{
	if (FMath::FRandRange(0.f, 1.f) <= CriticalChance)
	{
		return Damage * CriticalDamageMultiplier;
	}
	else return Damage;
}

void ASword::HitStop(float Duration, float TimeScale)
{
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(TimeScale);

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, this, &ASword::ResetTimeDilation, Duration, false);
	}
}

void ASword::ResetTimeDilation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetWorldSettings()->SetTimeDilation(1.0f);
	}
}