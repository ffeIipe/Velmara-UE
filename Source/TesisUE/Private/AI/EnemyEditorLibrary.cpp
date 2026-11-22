#include "AI/EnemyEditorLibrary.h"

#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

bool UEnemyEditorLibrary::CopyAttackNotifiesFromTemplate(UAnimMontage* Montage, UAnimMontage* Template, const float HitStart, const float HitEnd,
	const float ComboWindowEnd)
{
#if WITH_EDITOR
	if (!Montage || !Template || Montage == Template || HitEnd <= HitStart) return false;

	const float Length = Montage->GetPlayLength();
	auto Clamp = [Length](const float Time) { return FMath::Clamp(Time, 0.f, Length); };

	Montage->Modify();
	Montage->Notifies.Reset();
	Montage->AnimNotifyTracks.Reset();
	for (const FAnimNotifyTrack& Track : Template->AnimNotifyTracks)
	{
		Montage->AnimNotifyTracks.Add(FAnimNotifyTrack(Track.TrackName, Track.TrackColor));
	}

	for (const FAnimNotifyEvent& Source : Template->Notifies)
	{
		FAnimNotifyEvent Event = Source;
		Event.Guid = FGuid::NewGuid();
		Event.Notify = Source.Notify ? DuplicateObject<UAnimNotify>(Source.Notify, Montage) : nullptr;
		Event.NotifyStateClass = Source.NotifyStateClass ? DuplicateObject<UAnimNotifyState>(Source.NotifyStateClass, Montage) : nullptr;

		const UObject* NotifyObject = Source.NotifyStateClass ? static_cast<const UObject*>(Source.NotifyStateClass) : Source.Notify;
		const FString ClassName = NotifyObject ? NotifyObject->GetClass()->GetName() : FString();

		float Start = Clamp(Source.GetTriggerTime());
		float Duration = Source.GetDuration();

		if (ClassName.Contains(TEXT("SwordCollision")))
		{
			Start = HitStart;
			Duration = HitEnd - HitStart;
		}
		else if (ClassName.Contains(TEXT("ComboWindow")))
		{
			Start = HitEnd;
			Duration = FMath::Max(ComboWindowEnd - HitEnd, 0.1f);
		}
		else if (ClassName.Contains(TEXT("MotionWarping")))
		{
			Start = FMath::Min(0.05f, HitStart * 0.5f);
			Duration = FMath::Max(HitStart - Start, 0.05f);
		}
		else if (ClassName.Contains(TEXT("TimedNiagaraEffect")))
		{
			Start = FMath::Max(HitStart - 0.03f, 0.f);
			Duration = HitEnd + 0.05f - Start;
		}
		else if (ClassName.Contains(TEXT("PlaySound")))
		{
			Start = FMath::Max(HitStart - 0.05f, 0.f);
		}

		Start = Clamp(Start);
		Event.Link(Montage, Start);
		Event.TriggerTimeOffset = GetTriggerTimeOffsetForType(Montage->CalculateOffsetForNotify(Start));

		if (Event.NotifyStateClass)
		{
			Duration = FMath::Clamp(Duration, 0.f, Length - Start);
			Event.SetDuration(Duration);
			Event.EndLink.Link(Montage, Start + Duration);
		}

		Montage->Notifies.Add(Event);
	}

	Montage->BlendIn = Template->BlendIn;
	Montage->BlendOut = Template->BlendOut;
	Montage->BlendOutTriggerTime = Template->BlendOutTriggerTime;

	Montage->SortNotifies();
	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

int32 UEnemyEditorLibrary::SetChildBonesTranslationFromSkeleton(USkeleton* Skeleton, const FName ParentBone)
{
#if WITH_EDITOR
	if (!Skeleton) return 0;

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 ParentIndex = RefSkeleton.FindBoneIndex(ParentBone);
	if (ParentIndex == INDEX_NONE) return 0;

	Skeleton->Modify();
	int32 ChangedCount = 0;
	for (int32 BoneIndex = ParentIndex + 1; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		if (RefSkeleton.BoneIsChildOf(BoneIndex, ParentIndex))
		{
			Skeleton->SetBoneTranslationRetargetingMode(BoneIndex, EBoneTranslationRetargetingMode::Skeleton);
			++ChangedCount;
		}
	}
	Skeleton->MarkPackageDirty();
	return ChangedCount;
#else
	return 0;
#endif
}
