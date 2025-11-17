#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"

IMPLEMENT_CLASS(UAnimInstance)
UAnimInstance::UAnimInstance()
{
	AnimStateMachine.SetOwner(this);
}

void UAnimInstance::Tick(float DeltaSeconds)
{
	if (OwnerComponent && bPlay && Speed != 0 && CurrentState)
	{
		AnimStateMachine.Tick(DeltaSeconds);
		PrevTime = CurrentTime;
		CurrentTime += DeltaSeconds * Speed;
		NativeUpdateAnimation(DeltaSeconds);
		TriggerAnimNotifies(DeltaSeconds);
	}
}
void UAnimInstance::ChangeState(UAnimState* AnimState, UAnimTransition* AnimTransition)
{
	ChangeState(AnimState, AnimTransition->BlendTime);
}
void UAnimInstance::ChangeState(UAnimState* AnimState, float InTransitionTime)
{
	CurrentState = AnimState;
	TransitionTime = InTransitionTime;
	CurTransitionTime = TransitionTime;
	CachedPose.Pose = GetOwner()->GetPose();
	CurrentTime = 0;
	PrevTime = 0;
	SetSpeed(AnimState->Speed);
	SetLoop(AnimState->bLoop);
	SetPlay(true);
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{
    if (!OwnerComponent) { return; }

    if (!CurrentState || !CurrentState->AnimSequence) { return; }

    UAnimSequence* CurrentSequence = CurrentState->AnimSequence;
    TArray<FAnimNotifyEvent> TriggeredNotifies;
    CurrentSequence->GetAnimNotifiesInRange(PrevTime, CurrentTime, TriggeredNotifies);

    // Build sequence key once (use file path only for consistency)
    FString SequenceKey;
    if (CurrentSequence)
    {
        SequenceKey = CurrentSequence->GetFilePath();
    }

    for (const FAnimNotifyEvent& Notify : TriggeredNotifies)
    {
        // Broadcast via component delegate for game-side handling with sequence key
        OwnerComponent->OnAnimNotify.Broadcast(Notify, SequenceKey);
    }
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	float TransitionBlendFactor = Clamp((TransitionTime - CurTransitionTime) / TransitionTime);
	CurTransitionTime -= CurrentTime - PrevTime;
	UAnimSequence* AnimSequence = CurrentState->AnimSequence;
	float SequenceTime = AnimSequence->GetSequenceLength();
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

	FPoseContext PoseContext(this);
	PoseContext.SetPose(AnimSequence, CurrentTime);

	if (TransitionBlendFactor < 1)
	{
		FPoseContext::BlendTwoPoses(CachedPose, PoseContext, TransitionBlendFactor, PoseContext);
	}

	OwnerComponent->SetPose(PoseContext);	
}
