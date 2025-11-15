#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"

void UAnimInstance::Tick(float DeltaSeconds)
{
	if (OwnerComponent && bPlay && Speed != 0)
	{
		PrevTime = CurrentTime;
		CurrentTime += DeltaSeconds * Speed;
		NativeUpdateAnimation(DeltaSeconds);
		TriggerAnimNotifies(DeltaSeconds);
	}
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{

}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	
}