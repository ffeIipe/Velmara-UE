#include "Enemy/Paladin.h"
#include "Player/PlayerMain.h"

#include "GameFramework/DamageType.h"
#include "DamageTypes/MeleeDamage.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/GameplayStatics.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include <Kismet/KismetMathLibrary.h>

#include "DataAssets/EntityData.h"
#include "Interfaces/Weapon/WeaponInterface.h"

#include "DataAssets/MontagesData.h"


APaladin::APaladin()
{
	WeaponToEquip = CreateDefaultSubobject<UChildActorComponent>(TEXT("Weapon"));
	WeaponToEquip->SetupAttachment(GetMesh());
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);

	OnShieldTakeDamage.AddDynamic(this, &APaladin::ShieldHit);
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();

	if (!BBComponent)
	{
		if (AIController)
		{
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}
		else
		{
			AIController = Cast<AAIController>(GetController());
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}
	}

	if (!WeaponToEquip->GetChildActor())
	{
		WeaponToEquip->CreateChildActor();
		//GetCharacterStateComponent_Implementation()->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	}
	
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	
	for (AActor* AttachedActor : AttachedActors)
	{
		if (const TScriptInterface<IWeaponInterface> WeaponInterface = AttachedActor)
		{
			if (!GetInventoryComponent()->TryAddWeapon(WeaponInterface))
			{
				WeaponToEquip->DestroyChildActor();
			}
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not valid weapon attached to actor.");
		}
	}
}

void APaladin::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (EntityData)
	{
		if (EntityData->InitialWeapon)
		{
			WeaponToEquip->SetChildActorClass(EntityData->InitialWeapon);
		}
	}
}

void APaladin::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	Super::ActivateEnemy(Location, Rotation);

	//CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APaladin::DeactivateEnemy()
{
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::DeactivateEnemy();
}

void APaladin::LaunchUp()
{
	/*if (bIsLaunched) return;*/
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Super::LaunchUp();
	
	bIsLaunched = true;

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, GetName() + " was launched.");
	DisableAI();
	PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("FromAir"));
}

void APaladin::Die(UAnimMontage* DeathAnim, const FName Section)
{
	Super::Die(DeathAnim, Section);
	
	NotifyDamageTakenToBlackboard(LastDamageCauser);
}

void APaladin::ShieldHit()
{
	if (MontagesData->Montages.HitReactMontage)
	{
		StopAnimMontage(GetCurrentMontage());
		PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("ShieldHit"));
	}
}

void APaladin::CrashDown()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("KnockDown"));
	LaunchCharacter(FVector(0.f, 0.f, -100000.f), true, true);
}

void APaladin::HitInAir()
{
	const float PlayerLocationHeight = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation().Z;
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, PlayerLocationHeight));
	PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("FromAir"));
	DisableAI();
}

void APaladin::ReactToDamage(const EMeleeDamageTypes DamageType, const FVector& ImpactPoint)
{
	if (!IsAlive()) return;
	
	switch (DamageType)
	{
	case EMeleeDamageTypes::EMDT_CrashDown:
		CrashDown();
		break;

	case EMeleeDamageTypes::EMDT_InAir:
		HitInAir();
		break;

	case EMeleeDamageTypes::EMDT_Finisher:
		FinishedDamage();
		break;

	case EMeleeDamageTypes::EMDT_Slash:
		Slash();
		break;

	case EMeleeDamageTypes::EMDT_Puncture:
		PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("PunctureReact"));
		break;

	case EMeleeDamageTypes::EMDT_Impact:
		PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("ImpactReact"));
		break;

	/*case EMainDamageTypes::EMDT_Pistol:*/

	case EMeleeDamageTypes::EMDT_Launch:
		LaunchUp();
		break;
		
	case EMeleeDamageTypes::EMDT_None:
		GetDirectionalReact(ImpactPoint);
		break;

	default:
		GetDirectionalReact(ImpactPoint);
		break;
	}
}

void APaladin::Slash()
{
	if (!IsAlive()) return;
	
	const FRotator DamageCauserLocation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LastDamageCauser->GetActorLocation());

	SetActorRotation(FRotator(0.f, DamageCauserLocation.Yaw, 0.f));

	StopAnimMontage(GetCurrentMontage());
	PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, FName("FromFrontBig"));
}
