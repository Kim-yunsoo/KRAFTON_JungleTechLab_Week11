#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimState.h"

IMPLEMENT_CLASS(UAnimState)

UAnimState::UAnimState()
{
	UE_LOG("%d AnimState Create", UUID);
}

UAnimState::~UAnimState()
{
	UE_LOG("%d AnimState Delete", UUID);
}
