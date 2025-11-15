#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"

class UAnimInstance : public UObject
{
	DECLARE_CLASS(UAnimInstance, UObject)
public:
	UAnimInstance() = default;
	virtual ~UAnimInstance() = default;
	void TriggerAnimNotifies(float DeltaSeconds);
	void Tick(float DeltaSeconds);

	void SetAnimation(UAnimSequence* InSequence, bool IsInit = true);
	void SetOwner(USkeletalMeshComponent* InOwner)
	{
		OwnerComponent = InOwner;
	}
	void SetLoop(const bool InLoop)
	{
		bLoop = InLoop;
	}
	void SetSpeed(const float InSpeed)
	{
		Speed = InSpeed;
	}
	void SetPosition(const float InPosition)
	{
		CurrentTime = InPosition;
	}
	void SetPlay(const bool InPlay)
	{
		bPlay = InPlay;
	}

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds);
	float CurrentTime = 0;
	float Speed = 1;
	bool bLoop = false;
	bool bPlay = false;
	USkeletalMeshComponent* OwnerComponent = nullptr;
	UAnimSequence* AnimSequence = nullptr;
private:


};