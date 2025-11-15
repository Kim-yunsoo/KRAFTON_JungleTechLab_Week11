#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimBlendInstance.h"

void UAnimBlendInstance::SetBlendAnimation(UAnimSequence* SequenceA, UAnimSequence* SequenceB)
{
	AnimA = SequenceA;
	AnimB = SequenceB;
}

void UAnimBlendInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (AnimA && AnimB)
	{
		float ASequenceTime = AnimA->GetSequenceLength();
		float BSequenceTime = AnimB->GetSequenceLength();
		float MaxSequenceTime = fmax(ASequenceTime, BSequenceTime);

		if (bLoop)
		{
			CurrentTime = ClampTimeLooped(CurrentTime, CurrentTime - PrevTime, MaxSequenceTime);
		}
		else
		{
			CurrentTime = Clamp(CurrentTime, 0, MaxSequenceTime);
			if (CurrentTime != MaxSequenceTime)
			{
				bPlay = false;
			}
		}

		FPoseContext PoseA(this);
		FPoseContext PoseB(this);
		PoseA.SetPose(AnimA, CurrentTime);
		PoseB.SetPose(AnimB, CurrentTime);
		FPoseContext::BlendTwoPoses(PoseA, PoseB, BlendAlpha, PoseA);
		OwnerComponent->SetPose(PoseA);
	}
}