#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"

void UAnimInstance::Tick(float DeltaSeconds)
{
	if (OwnerComponent && AnimSequence && bPlay)
	{
		NativeUpdateAnimation(DeltaSeconds);
	}
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{

}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{

}

void UAnimInstance::SetAnimation(UAnimSequence* InSequence, bool IsInit)
{
	AnimSequence = InSequence;
	if (IsInit)
	{
		SetLoop(false);
		SetSpeed(1);
		SetPosition(0);
	}
}