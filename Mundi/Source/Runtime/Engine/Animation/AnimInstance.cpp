#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

void UAnimInstance::Tick(float DeltaSeconds)
{
	if (OwnerComponent && AnimSequence && bPlay)
	{
		float MaxTime = AnimSequence->GetPlayLength();
		if (CurrentTime >= MaxTime)
		{
			if (bLoop)
			{
				CurrentTime = 0;
			}
			else
			{
				CurrentTime = MaxTime;
				bPlay = false;
			}
		}

		const TArray<FBoneAnimationTrack>& BoneTracks = AnimSequence->GetBoneTracks();
		float FrameRate = AnimSequence->GetFrameRate();
		for (const FBoneAnimationTrack& BoneTrack : BoneTracks)
		{
			FTransform BoneTransform = BoneTrack.InternalTrack.GetTransform(FrameRate, CurrentTime);
			OwnerComponent->SetBoneLocalTransform(BoneTrack.BoneIndex, BoneTransform);
		}
	}
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{

}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{

}

void UAnimInstance::SetAnimation(UAnimSequence* InSequence, bool IsInit)
{
	AnimSequence = InSequence;
	if (IsInit)
	{
		SetLoop(false);
		SetSpeed(1);
		SetPosition(0);
	}
}