#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"

class UAnimInstance : public UObject
{
	DECLARE_CLASS(UAnimInstance, UObject)
public:
	UAnimInstance() = default;
	virtual ~UAnimInstance() = default;
	void TriggerAnimNotifies(float DeltaSeconds);
	void Tick(float DeltaSeconds);

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
	void SetTime(const float InTime)
	{
		CurrentTime = InTime;
	}
	void SetPlay(const bool InPlay)
	{
		bPlay = InPlay;
	}
	USkeletalMeshComponent* GetOwner() const { return OwnerComponent; }

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds);
	float CurrentTime = 0;
	float PrevTime = 0;
	float Speed = 1;
	bool bLoop = false;
	bool bPlay = false;
	USkeletalMeshComponent* OwnerComponent = nullptr;
private:


};