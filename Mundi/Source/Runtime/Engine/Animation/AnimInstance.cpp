#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

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

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{

}

//반복없는 애니메이션 끝나면 조건없는 트랜지션을 타고 이동 가능하도록 제작 필요 (애니메이션이 끝날때 라는 조건인거임)
void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	float TransitionBlendFactor = Clamp((TransitionTime - CurTransitionTime) / TransitionTime);
	CurTransitionTime -= abs(CurrentTime - PrevTime);

	float SequenceTime = CurrentState->GetTotalSequenceTime();
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
	CurrentState->GetStatePose(this, PoseContext, CurrentTime);

	if (TransitionBlendFactor < 1)
	{
		FPoseContext::BlendTwoPoses(CachedPose, PoseContext, TransitionBlendFactor, PoseContext);
	}

	OwnerComponent->SetPose(PoseContext);	
}