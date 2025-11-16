#pragma once
#include "Source/Runtime/Engine/Animation/AnimState.h"
#include "Source/Runtime/Engine/Animation/AnimTransition.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"

class UAnimInstance;

class UAnimStateMachine : public UObject
{
	DECLARE_CLASS(UAnimStateMachine, UObject)

public:
	void SetOwner(UAnimInstance* InOwner)
	{
		Owner = InOwner;
	}
	void StartStateMachine(UAnimState* StartState);
	void AddState(UAnimState* InState);
	void RemoveState(UAnimState* InState);
	void AddTransition(UAnimTransition* InTransition);
	void RemoveTransition(UAnimTransition* InTransition);
	void Tick(float DeltaSeconds);
	void SetCurrentState(UAnimState* InAnimState, UAnimTransition* InTransition);

private:
	UAnimInstance* Owner;
	UAnimState* CurrentState = nullptr;
	TArray<UAnimState*> States;
	TArray<UAnimTransition*> Transitions;
};