#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"

void UAnimSingleNodeInstance::SetAnimSequence(UAnimSequence* InAnimSequence, const bool bLoop)
{
	AnimSequence = InAnimSequence;
	SetLoop(bLoop);
	SetTime(0);
	SetSpeed(1);
	SetPlay(true);
}


void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	float SequenceTime = AnimSequence->GetPlayLength();
	float ClampTime = Clamp(CurrentTime, 0, SequenceTime);
	//범위 벗어났을 경우
	if (ClampTime != CurrentTime)
	{
		if (bLoop)
		{
			float Quotient = CurrentTime / SequenceTime;
			float FracQuotient = Quotient - floor(Quotient);
			CurrentTime = FracQuotient * SequenceTime;
		}
		else
		{
			CurrentTime = ClampTime;
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
