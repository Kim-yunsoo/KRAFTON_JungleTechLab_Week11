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
	if (RegistChangeState != CurrentState)
	{
		ChangeState();
	}
	if (OwnerComponent && bPlay && Speed != 0 && CurrentState)
	{
		AnimStateMachine.Tick(DeltaSeconds);
		PrevTime = CurrentTime;
		CurrentTime += DeltaSeconds * Speed;
		NativeUpdateAnimation(DeltaSeconds);
		TriggerAnimNotifies(DeltaSeconds);	
	}
}
void UAnimInstance::ChangeState()
{
	CurrentState = RegistChangeState;
	TransitionTime = RegistChangeTransitionTime;
	CurTransitionTime = TransitionTime;
	CachedPose.Pose = GetOwner()->GetPose();
	CurrentTime = 0;
	PrevTime = 0;
	SetSpeed(CurrentState->Speed);
	Play();
}
void UAnimInstance::ChangeStatePending(UAnimState* AnimState, UAnimTransition* AnimTransition)
{
	ChangeStatePending(AnimState, AnimTransition->BlendTime);
}
void UAnimInstance::ChangeStatePending(UAnimState* AnimState, float InTransitionTime)
{
	RegistChangeState = AnimState;
	RegistChangeTransitionTime = InTransitionTime;
}

UAnimState* UAnimInstance::AddSequenceInState(const FString& InName, UAnimSequence* Sequence, const float InBlendValue)
{
	return AnimStateMachine.AddSequenceInState(InName, Sequence, InBlendValue);
}
UAnimState* UAnimInstance::AddSequenceInState(const FString& InName, const FString& AnimPath, const float InBlendValue)
{
	UAnimSequence* Sequence = RESOURCE.Get<UAnimSequence>(AnimPath);
	if (Sequence == nullptr)
	{
		UE_LOG("Anim None %s", AnimPath);
		return nullptr;
	}
	return AnimStateMachine.AddSequenceInState(InName, Sequence, InBlendValue);
}

void UAnimInstance::SetBlendValueInState(const FString& InName, const float InBlendValue)
{
	UAnimState* State = AnimStateMachine.GetState(InName);
	if (State)
	{
		State->SetBlnedValue(InBlendValue);
	}
}

UAnimTransition* UAnimInstance::AddTransition(const FString& StartStateName, const FString& EndStateName)
{
	return AnimStateMachine.AddTransition(StartStateName, EndStateName);
}
void UAnimInstance::SetStartState(const FString& StartStateName)
{
	AnimStateMachine.StartStateMachine(StartStateName, 0);
}
void UAnimInstance::SetStateLoop(const FString& InName, const bool InLoop)
{
	UAnimState* State = AnimStateMachine.GetState(InName);
	if (State)
	{
		State->SetLoop(InLoop);
	}
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{
    if (!OwnerComponent)
    {
        UE_LOG("[AnimNotify] TriggerAnimNotifies: OwnerComponent is null!");
        return;
    }

    if (!CurrentState)
    {
        UE_LOG("[AnimNotify] TriggerAnimNotifies: CurrentState is null!");
        return;
    }
	TArray<UAnimSequence*> ActiveSequence = CurrentState->GetCurrentActiveSequence();

    if (ActiveSequence.Num() == 0)
    {
        UE_LOG("[AnimNotify] TriggerAnimNotifies: CurrentState->AnimSequence is null!");
        return;
    }


	for (UAnimSequence* Sequence : ActiveSequence)
	{
		TArray<FAnimNotifyEvent> TriggeredNotifies;
		Sequence->GetAnimNotifiesInRange(PrevTime, CurrentTime, TriggeredNotifies);

		// Build sequence key once (use file path only for consistency)
		FString SequenceKey;
		if (Sequence)
		{
			SequenceKey = Sequence->GetFilePath();
		}

		for (const FAnimNotifyEvent& Notify : TriggeredNotifies)
		{
			// Component-level delegate; actor or systems can forward to dispatcher/blueprints/etc.
			OwnerComponent->OnAnimNotify.Broadcast(Notify, SequenceKey);

			FNotifyDispatcher::Get().Dispatch(SequenceKey, Notify);
		}
	}
   
}

//반복없는 애니메이션 끝나면 조건없는 트랜지션을 타고 이동 가능하도록 제작 필요 (애니메이션이 끝날때 라는 조건인거임)
void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	float TransitionBlendFactor = Clamp((TransitionTime - CurTransitionTime) / TransitionTime);
	CurTransitionTime -= abs(CurrentTime - PrevTime);

	float SequenceTime = CurrentState->GetTotalSequenceTime();
	if (CurrentState->bLoop)
	{	
		CurrentTime = ClampTimeLooped(CurrentTime, CurrentTime - PrevTime, SequenceTime);
	}
	else
	{
		if (CurrentTime > SequenceTime)
		{
			bPlay = false;
			AnimStateMachine.EndState();
		}
		CurrentTime = Clamp(CurrentTime, 0.0f, SequenceTime);
	}

	FPoseContext PoseContext(this);
	CurrentState->GetStatePose(this, PoseContext, CurrentTime);

	if (TransitionBlendFactor < 1)
	{
		FPoseContext::BlendTwoPoses(CachedPose, PoseContext, TransitionBlendFactor, PoseContext);
	}

	OwnerComponent->SetPose(PoseContext);	
}
