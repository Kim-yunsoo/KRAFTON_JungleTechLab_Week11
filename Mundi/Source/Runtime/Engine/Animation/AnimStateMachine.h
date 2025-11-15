#pragma once
#include "Source/Runtime/Engine/Animation/AnimState.h"
#include "Source/Runtime/Engine/Animation/AnimTransition.h"

class UAnimStateMachine : public UObject
{
	DECLARE_CLASS(UAnimStateMachine, UObject)

public:
	void AddState(UAnimState* InState);
	void AddTransition(UAnimTransition* InTransition);
	void Tick(float DeltaSeconds);

public:
	UAnimState* CurrentState = nullptr;
	TArray<UAnimState*> States;
	TArray<UAnimTransition*> Transitions;
};