// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"

#include "Player/PlayerMain.h"
#include "Components/CombatComponent.h"

ASword::ASword()
{
	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box"));
	WeaponBox->SetupAttachment(GetRootComponent());
	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	WeaponBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

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
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);

	AttachMeshToSocket(InParent, InSocketName);
	ItemState = EItemState::EIS_Equipped;

	DisableCollision();
}

void ASword::AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName)
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

void ASword::Enable(bool Param)
{
	if (!ItemMesh) return;

	ItemMesh->SetVisibility(Param);
	SetActorHiddenInGame(!Param);
	SetActorEnableCollision(Param);

	APlayerMain* PlayerMain = Cast<APlayerMain>(Owner);

	if (!PlayerMain) return;
	
	if (Param)
	{
		PlayerMain->CombatComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword);
	}
	else
	{
		PlayerMain->CombatComponent->SetCharacterState(ECharacterStates::ECS_Unequipped);
	}
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

	TArray<FHitResult> HitResults;;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(10.f, 10.f, 10.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_EngineTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);
	
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && !IgnoreActors.Contains(HitActor))
		{
			if (IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
			{
				TSubclassOf<UDamageType> FinalDamageType = DamageTypeClass ? DamageTypeClass : TSubclassOf<UDamageType>(UDamageType::StaticClass());

				UGameplayStatics::ApplyDamage(
					HitActor,
					Damage,
					GetInstigator()->GetController(),
					GetOwner(),
					FinalDamageType
				);
				HitInterface->Execute_GetHit(HitActor, Hit.ImpactPoint);
				CameraShake();

				IgnoreActors.Add(HitActor);
			}
			else if (Hit.GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic)
			{
				OnWallHit.Broadcast(Hit);
			}
		}
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString(Hit.GetActor()->GetName()));
	}
}