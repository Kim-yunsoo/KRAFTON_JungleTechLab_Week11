#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "UAnimState.generated.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"

class UAnimSequence;

class UAnimState : public UObject
{
	GENERATED_REFLECTION_BODY()
public:
	UAnimState();
	virtual ~UAnimState();

	void SetName(const FString& InName)
	{
		Name = InName;
	}
	void AddSequence(UAnimSequence* InSequence, float InBlendValue = 0)
	{
		if (AnimSequenceMap.Contains(InBlendValue))
		{
			AnimSequenceMap.Remove(InBlendValue);
		}
		AnimSequenceMap.Add(InBlendValue, InSequence);
		TotalSequenceTime = 0;
		for (UAnimSequence* Sequence : AnimSequenceMap.GetValues())
		{
			float CurLength = Sequence->GetPlayLength();
			TotalSequenceTime = TotalSequenceTime < CurLength ? CurLength : TotalSequenceTime;
		}
	}
	float GetTotalSequenceTime() const { return TotalSequenceTime; }
	void SetSpeed(const float InSpeed)
	{
		Speed = InSpeed;
	}
	void SetStartTime(const float InStartTime)
	{
		StartTime = InStartTime;
	}
	void SetLoop(const bool InLoop)
	{
		bLoop = InLoop;
	}
	float GetBlendValue() const { return BlendValue; }
	void SetBlnedValue(const float InBlendValue) 
	{
		BlendValue = InBlendValue;
		TArray<float> SequenceBlendKeys = AnimSequenceMap.GetKeys();
		if (SequenceBlendKeys.Num() > 0)
		{
			float min = SequenceBlendKeys[0];
			float max = SequenceBlendKeys[0];
			for (float BlendValue : SequenceBlendKeys)
			{
				min = BlendValue < min ? BlendValue : min;
				max = BlendValue > max ? BlendValue : max;
			}

			BlendValue = Clamp(BlendValue, min, max);
		}
	}
	void GetStatePose(UAnimInstance* AnimInstance, FPoseContext& Pose, float CurrentTime);
	
public:
	FString Name;
	TOrderedMap<float, UAnimSequence*> AnimSequenceMap;

	float TotalSequenceTime = 0.0f;
	float BlendValue = 0.0f;
	float Speed = 1.0f;
	float StartTime = 0.0f;
	bool bLoop = true;


};