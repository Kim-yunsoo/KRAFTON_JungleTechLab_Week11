#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"
#include "Source/Runtime/Engine/Animation/AnimStateMachine.h"

class UAnimInstance : public UObject
{
	DECLARE_CLASS(UAnimInstance, UObject)
public:
	UAnimInstance();
	virtual ~UAnimInstance() = default;
	void TriggerAnimNotifies(float DeltaSeconds);
	void Tick(float DeltaSeconds);
	void ChangeState(UAnimState* AnimState, UAnimTransition* AnimTransition);
	void ChangeState(UAnimState* AnimState, float InTransitionTime);

	UAnimState* AddState(const FString& InName, UAnimSequence* Sequence);
	UAnimTransition* AddTransition(const FString& StartStateName, const FString& EndStateName);
	void SetStartState(const FString& StartStateName, const float InBlendTime = 0.0f);

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
	UAnimStateMachine& GetStateMachine()
	{
		return AnimStateMachine;
	}
	USkeletalMeshComponent* GetOwner() const { return OwnerComponent; }

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds);
	float CurrentTime = 0;
	float PrevTime = 0;
	float Speed = -1;
	bool bLoop = false;
	bool bPlay = false;
	USkeletalMeshComponent* OwnerComponent = nullptr;
	UAnimStateMachine AnimStateMachine;

private:

	FPoseContext CachedPose;
	UAnimState* CurrentState = nullptr;
	float TransitionTime = 0;
	float CurTransitionTime = 0;

};