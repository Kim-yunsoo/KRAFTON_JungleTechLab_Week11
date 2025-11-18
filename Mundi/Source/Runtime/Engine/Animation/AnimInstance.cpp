#include "pch.h"
#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

#include "Source/Runtime/Engine/Animation/NotifyDispatcher.h"
IMPLEMENT_CLASS(UAnimInstance)
UAnimInstance::UAnimInstance()
{
	UE_LOG("%d AnimInstance Create", UUID);
	AnimStateMachine.SetOwner(this);
}
UAnimInstance::~UAnimInstance()
{
	UE_LOG("%d AnimInstance Destroy", UUID);
}

void UAnimInstance::Tick(float DeltaSeconds)
{
	int test = 0;
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
	Play();
}

UAnimState* UAnimInstance::AddState(const FString& InName, UAnimSequence* Sequence)
{
	return AnimStateMachine.AddState(InName, Sequence);;
}
UAnimState* UAnimInstance::AddState(const FString& InName, const FString& AnimPath)
{
	UAnimSequence* Sequence = RESOURCE.Get<UAnimSequence>(AnimPath);
	if (Sequence == nullptr)
	{
		UE_LOG("Anim None %s", AnimPath);
		return nullptr;
	}
	return AnimStateMachine.AddState(InName, Sequence);
}
UAnimTransition* UAnimInstance::AddTransition(const FString& StartStateName, const FString& EndStateName)
{
	return AnimStateMachine.AddTransition(StartStateName, EndStateName);
}
void UAnimInstance::SetStartState(const FString& StartStateName)
{
	AnimStateMachine.StartStateMachine(StartStateName, 0);
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
        // Component-level delegate; actor or systems can forward to dispatcher/blueprints/etc.
        OwnerComponent->OnAnimNotify.Broadcast(Notify, SequenceKey);
        FNotifyDispatcher::Get().Dispatch(SequenceKey, Notify);
    }
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	float TransitionBlendFactor = Clamp((TransitionTime - CurTransitionTime) / TransitionTime);
	CurTransitionTime -= abs(CurrentTime - PrevTime);
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
