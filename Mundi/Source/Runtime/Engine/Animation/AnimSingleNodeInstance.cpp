#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

IMPLEMENT_CLASS(UAnimSingleNodeInstance)

void UAnimSingleNodeInstance::SetAnimSequence(UAnimSequence* InAnimSequence, const bool bLoop)
{
	AnimSequence = InAnimSequence;
	SetLoop(bLoop);
	SetTime(0);
	SetSpeed(1);
	Play();
}

void UAnimSingleNodeInstance::Tick(float DeltaSeconds)
{
	if (OwnerComponent && bPlay && Speed != 0 && AnimSequence)
	{
		PrevTime = CurrentTime;
		CurrentTime += DeltaSeconds * Speed;
		NativeUpdateAnimation(DeltaSeconds);
		TriggerAnimNotifies(DeltaSeconds);
	}
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
		CurrentTime = Clamp(CurrentTime, 0.0f, SequenceTime);
		if (CurrentTime != SequenceTime)
		{
			bPlay = false;
		}
	}

	FPoseContext PoseA(this);
	PoseA.SetPose(AnimSequence, CurrentTime);
	OwnerComponent->SetPose(PoseA);
}
