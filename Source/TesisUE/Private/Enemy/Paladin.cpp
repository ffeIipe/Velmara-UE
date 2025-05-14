#include "Enemy/Paladin.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/DamageTypeMain.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CombatComponent.h"
#include "Components/CharacterStateComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Interfaces/HitInterface.h"
#include "EnhancedInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"


APaladin::APaladin()
{
	Attributes->AttachShield(GetMesh(), FName("LeftHandSocket"));

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
	SwordMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	SwordCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordBoxCollider"));
	SwordCollider->SetupAttachment(SwordMesh);
	SwordCollider->SetRelativeLocation(FVector(0.f, 0.f, 48.f));
	SwordCollider->SetBoxExtent(FVector(3.f, 2.f, 36.f));
	SwordCollider->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 11.f));

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(SwordMesh);
	BoxTraceStart->SetRelativeLocation(FVector(0.f, 0.f, 82.f));

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterStateComponent"));
}

bool APaladin::IsLaunchable_Implementation()
{
	return !Attributes->IsShielded();
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();
	
	SwordCollider->OnComponentBeginOverlap.AddDynamic(this, &APaladin::OnSwordOverlap);

	CharacterStateComponent->SetCharacterState(ECharacterStates::ECS_EquippedSword);
}

void APaladin::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (SwordCollider)
	{
		SwordCollider->SetCollisionEnabled(CollisionEnabled);
	}
}

void APaladin::OnSwordOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

	IHitInterface* Entity = Cast<IHitInterface>(BoxHit.GetActor());
	if (Entity/* && Player->GetCharacterAction() != ECharacterActions::ECA_Block*/)
	{
		UGameplayStatics::ApplyDamage(
			BoxHit.GetActor(),
			Damage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
		);
		ActorsToIgnore.AddUnique(BoxHit.GetActor());
		
		Entity->Execute_GetHit(BoxHit.GetActor(), BoxHit.ImpactPoint);
	}
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

	EnhancedInputComponent->BindAction(CombatComponent->AttackAction, ETriggerEvent::Triggered, this, &APaladin::Attack);
}

void APaladin::ShieldHit()
{
	PlayAnimMontage(HitReactMontage, 1.f, FName("ShieldHit"));
}

void APaladin::Attack(const FInputActionValue& Value)
{
	CombatComponent->Input_Attack(Value);
}

void APaladin::LaunchUp_Implementation(const FVector& InstigatorLocation)
{
	if (!Attributes->IsShielded()) LaunchEnemyUp(InstigatorLocation);
}

void APaladin::GetHit_Implementation(const FVector& ImpactPoint)
{
	Super::GetHit_Implementation(ImpactPoint);

	if (Attributes && Attributes->IsAlive() && GetEnemyState() != EEnemyState::EES_Died)
	{
		ReactToDamage(LastDamageType, ImpactPoint);
	}
	else
	{
		PlayAnimMontage(DeathMontage, 1.f, SelectRandomDieAnim());
		Die();
	}
}

UCharacterStateComponent* APaladin::GetCharacterStateComponent_Implementation()
{
	return CharacterStateComponent;
}

float APaladin::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Attributes->IsAlive())
	{
		if (Attributes->GetShieldMesh())
		{
			if (Attributes->IsShielded())
			{				
				Attributes->ReceiveShieldDamage(DamageAmount);
				ShieldHit();
			}
			else if (!Attributes->bIsDisarmed) Attributes->DettachShield();

			else Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		}
		else Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
	return DamageAmount;
}

void APaladin::LaunchEnemyUp(const FVector& InstigatorLocation)
{
	if (isLaunched) return;

	isLaunched = true;
	DisableAI();
	PlayAnimMontage(HitReactMontage, 1.f, FName("FromAir"));
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, InstigatorLocation.Z));
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

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.F, FColor::Emerald, FString("APaladin::HitInAir"));
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