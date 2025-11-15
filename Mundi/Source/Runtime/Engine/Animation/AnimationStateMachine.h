#pragma once
#include "Source/Runtime/Core/Object/Object.h"

enum EAnimState
{
	AS_Idle,
	AS_Work,
	AS_Run,
	AS_Fly
};

enum EMovementMode
{
	MM_Walking,
	MM_Flying,
};

class UAnimationStateMachine : public UObject
{

	void ProcessState();
};