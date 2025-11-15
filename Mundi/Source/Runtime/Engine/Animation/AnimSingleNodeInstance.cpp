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
	if (bLoop)
	{
		CurrentTime = ClampTimeLooped(CurrentTime, CurrentTime - PrevTime, SequenceTime);
	}
	else
	{
		CurrentTime = Clamp(CurrentTime, 0, SequenceTime);
		if (CurrentTime != SequenceTime)
		{
			bPlay = false;
		}
	}

	FPoseContext PoseA(this);
	PoseA.SetPose(AnimSequence, CurrentTime);
	OwnerComponent->SetPose(PoseA);
}
