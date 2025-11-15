#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"


void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
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
	TArray<FTransform> BoneLocalTransforms;
	BoneLocalTransforms.Reserve(BoneTracks.Num());
	for (const FBoneAnimationTrack& BoneTrack : BoneTracks)
	{
		BoneLocalTransforms.Push(BoneTrack.InternalTrack.GetTransform(FrameRate, CurrentTime));
	}
	OwnerComponent->SetPose(BoneLocalTransforms);
}
