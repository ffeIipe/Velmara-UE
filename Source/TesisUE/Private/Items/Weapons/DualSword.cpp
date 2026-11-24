#include "Items/Weapons/DualSword.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ADualSword::ADualSword()
{
	OffHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OffHand Mesh"));
	OffHandMesh->SetupAttachment(GetRootComponent());
	OffHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OffHandDamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OffHand Weapon Box"));
	OffHandDamageBox->SetupAttachment(OffHandMesh);
	OffHandDamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OffHandDamageBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	OffHandDamageBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	OffHandDamageBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OffHandDamageBox->OnComponentBeginOverlap.AddDynamic(this, &ADualSword::OnOffHandOverlap);

	OffHandTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("OffHand Trace Start"));
	OffHandTraceStart->SetupAttachment(OffHandMesh);

	OffHandTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("OffHand Trace End"));
	OffHandTraceEnd->SetupAttachment(OffHandMesh);
}

void ADualSword::AttachMeshToSocket(USceneComponent* InParent, const FName InSocketName)
{
	Super::AttachMeshToSocket(InParent, InSocketName);

	if (!OffHandMesh->GetStaticMesh())
	{
		OffHandMesh->SetStaticMesh(ItemMesh->GetStaticMesh());
	}

	OffHandMesh->SetMaterial(0, ItemMesh->GetMaterial(0));

	const bool bEquipping = InSocketName.IsNone() || InSocketName == GetEquipSocketName();

	const FAttachmentTransformRules TransformRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		true
		);

	OffHandMesh->AttachToComponent(InParent, TransformRules, bEquipping ? OffHandSocketName : OffHandHolsterSocketName);
}

void ADualSword::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	CollisionState = CollisionEnabled;
	ApplyCollision();
}

void ADualSword::SetActiveHand(const EDualSwordHand NewHand)
{
	ActiveHand = NewHand;
	ApplyCollision();
}

void ADualSword::ApplyCollision()
{
	const bool bRightActive = ActiveHand != EDualSwordHand::Left;
	const bool bLeftActive = ActiveHand != EDualSwordHand::Right;

	WeaponDamageBox->SetCollisionEnabled(bRightActive ? CollisionState : ECollisionEnabled::NoCollision);
	OffHandDamageBox->SetCollisionEnabled(bLeftActive ? CollisionState : ECollisionEnabled::NoCollision);
}

void ADualSword::EnableVisuals()
{
	Super::EnableVisuals();
	OffHandMesh->SetVisibility(true);
}

void ADualSword::DisableVisuals()
{
	Super::DisableVisuals();
	OffHandMesh->SetVisibility(false);
}

void ADualSword::OnOffHandOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TraceAndApplyHits(OverlappedComponent, OffHandTraceStart, OffHandTraceEnd);
}
