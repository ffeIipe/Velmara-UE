// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Paladin.h"
#include "Components/BoxComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Player/PlayerMain.h>

APaladin::APaladin()
{
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));

	SwordBoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordBoxCollider->SetupAttachment(SwordMesh);
	SwordBoxCollider->SetRelativeLocation(FVector(0.f, 0.f, 48.f));
	SwordBoxCollider->SetBoxExtent(FVector(3.f, 2.f, 36.f));

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 11.f));

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 82.f));
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();
	
	SwordBoxCollider->OnComponentBeginOverlap.AddDynamic(this, &APaladin::OnBoxOverlap);
}

void APaladin::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	FHitResult BoxHit;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	UKismetSystemLibrary::BoxTraceSingle(
		this,
		Start,
		End,
		FVector(5.f, 5.f, 5.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		BoxHit,
		true
	);

	APlayerMain* Player = Cast<APlayerMain>(BoxHit.GetActor());
	if (Player)
	{
		UGameplayStatics::ApplyDamage(
			BoxHit.GetActor(),
			Damage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
		);
		ActorsToIgnore.AddUnique(BoxHit.GetActor());
	}

	//TODO: fx to play if player has been damaged
	/*if (APlayerMain* Player = Cast<APlayerMain>(BoxHit.GetActor()))
	{
		Player->GetHit()
	}*/
}