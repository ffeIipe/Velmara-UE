#include "Enemy/Paladin.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/DamageTypeMain.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Player/PlayerMain.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"


APaladin::APaladin()
{
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
	SwordMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	SwordBoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordBoxCollider->SetupAttachment(SwordMesh);
	SwordBoxCollider->SetRelativeLocation(FVector(0.f, 0.f, 48.f));
	SwordBoxCollider->SetBoxExtent(FVector(3.f, 2.f, 36.f));
	SwordBoxCollider->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

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

void APaladin::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (SwordBoxCollider)
	{
		SwordBoxCollider->SetCollisionEnabled(CollisionEnabled);
	}
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

	//APlayerMain* Player = Cast<APlayerMain>(BoxHit.GetActor());
	//if (Player && Player->GetCharacterAction() != ECharacterActions::ECA_Block)
	//{
	//	UGameplayStatics::ApplyDamage(
	//		BoxHit.GetActor(),
	//		Damage,
	//		GetInstigator()->GetController(),
	//		this,
	//		UDamageType::StaticClass()
	//	);
	//	ActorsToIgnore.AddUnique(BoxHit.GetActor());
	//}
	//else if (Player)
	//{
	//	PlayAnimMontage(HitReactMontage, 1.f, FName("FromFront"));
	//	Player->ReceiveBlock();
	//}

	//TODO: fx to play if player has been damaged
	/*if (APlayerMain* Player = Cast<APlayerMain>(BoxHit.GetActor()))
	{
		Player->GetHit()
	}*/
}

void APaladin::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &APaladin::Attack);
}

void APaladin::Attack()
{
	PerformAttackEvent();
}

bool APaladin::IsLaunchable_Implementation()
{
	return true;
}

void APaladin::LaunchUp_Implementation()
{
	LaunchEnemyUp();
}

AActor* APaladin::GetInterfaceOwner_Implementation()
{
	return GetOwner();
}

void APaladin::LaunchEnemyUp()
{
	if (isLaunched) return;

	isLaunched = true;
	DisableAI();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	AddActorWorldOffset(FVector(0.f, 0.f, 300.f), false);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void APaladin::CrashDown()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	PlayAnimMontage(HitReactMontage, 1.f, FName("KnockDown"));
	LaunchCharacter(FVector(0.f, 0.f, -100000.f), true, true);
}

void APaladin::HitInAir()
{
	float PlayerLocationZ = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation().Z;
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, PlayerLocationZ));
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	GetCharacterMovement()->IsFlying();
	DisableAI();
}

void APaladin::ReactToDamage(EMainDamageTypes DamageType, const FVector& ImpactPoint)
{
	Super::ReactToDamage(DamageType, ImpactPoint);

	switch (DamageType)
	{
	case EMainDamageTypes::EMDT_CrashDown:
		CrashDown();
		break;

	case EMainDamageTypes::EMDT_InAir:
		HitInAir();
		break;

	case EMainDamageTypes::EMDT_Finisher:
		return;
		break;
	default:
		DirectionalHitReact(ImpactPoint);
		break;
	}
}
