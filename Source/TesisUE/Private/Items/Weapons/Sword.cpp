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
	AttachMeshToSocket(InParent, InSocketName);
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	ItemState = EItemState::EIS_Equipped;
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

void ASword::Unequip()
{
	Super::Unequip();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ItemState = EItemState::EIS_Hovering;

	UE_LOG(LogTemp, Log, TEXT("Sword %s unequipped"), *GetName());
}

void ASword::EnableVisuals(bool bEnable)
{
	Super::EnableVisuals(bEnable);

	if (ItemMesh)
	{
		ItemMesh->SetVisibility(bEnable);
	}
	if (WeaponBox)
	{
		WeaponBox->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	APlayerMain* PlayerOwner = Cast<APlayerMain>(GetOwner());
	if (PlayerOwner && PlayerOwner->CombatComponent)
	{
		if (bEnable)
		{
			PlayerOwner->CombatComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword);
		}
		else
		{
			if (ItemState == EItemState::EIS_Equipped)
			{
				PlayerOwner->CombatComponent->SetCharacterState(ECharacterStates::ECS_Unequipped);
			}
		}
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

	TArray<FHitResult> HitResults;;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(10.f, 10.f, 10.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true
	);
	
	if (HitResults.Num() <= 0) return;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && IgnoreActors.Contains(HitActor)) return;
		
		IHitInterface* HitInterface = Cast<IHitInterface>(HitActor);

		if (HitInterface && HitInterface->Execute_IsLaunchable(HitActor))
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
		else if(HitInterface && !HitInterface->Execute_IsLaunchable(HitActor))
		{
			OnWallHit.Broadcast(Hit);
			HitInterface->Execute_ShieldHit(HitActor);
		}
	}
}