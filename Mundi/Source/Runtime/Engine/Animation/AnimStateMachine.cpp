#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimStateMachine.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"


IMPLEMENT_CLASS(UAnimStateMachine)

void UAnimStateMachine::Tick(float DeltaSeconds)
{
	if (CurrentState == nullptr)
	{
		return;
	}

	for (UAnimTransition* Transition : Transitions)
	{
		if (Transition->From == CurrentState && Transition->CanEnter())
		{
			SetCurrentState(Transition->To, Transition);
			break;
		}
	}
}
void UAnimStateMachine::SetCurrentState(UAnimState* InAnimState, UAnimTransition* InTransition)
{
	CurrentState = InAnimState;
	Owner->ChangeState(CurrentState, InTransition);
}

void UAnimStateMachine::StartStateMachine(UAnimState* StartState)
{
	if (States.Contains(StartState) == false)
	{
		//없으면 트랜지션이 연결 안되어 있기 때문에 안되도록 처리했음
		return;
	}
	CurrentState = StartState;
	Owner->ChangeState(CurrentState, 0.2f);
}

void UAnimStateMachine::AddState(UAnimState* InState)
{
	if (States.Contains(InState) == false)
	{
		States.Push(InState);
	}
}
void UAnimStateMachine::RemoveState(UAnimState* InState)
{
	States.Remove(InState);
	int TransitionCount = Transitions.Num();
	for (int i = TransitionCount - 1; i >= 0; i--)
	{
		UAnimTransition* Transition = Transitions[i];
		if (Transition->To == InState || Transition->From == InState)
		{
			Transitions.RemoveAt(i);
		}
	}
}
void UAnimStateMachine::AddTransition(UAnimTransition* InTransition)
{
	if (Transitions.Contains(InTransition) == false)
	{
		Transitions.Push(InTransition);
	}
}
void UAnimStateMachine::RemoveTransition(UAnimTransition* InTransition)
{
	Transitions.Remove(InTransition);
}