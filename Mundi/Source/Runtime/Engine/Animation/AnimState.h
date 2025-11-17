#pragma once
#include "Source/Runtime/Core/Object/Object.h"
#include "UAnimState.generated.h"

class UAnimSequence;

class UAnimState : public UObject
{
	GENERATED_REFLECTION_BODY()
public:
	UAnimState() = default;
	virtual ~UAnimState() = default;

	void SetName(const FString& InName)
	{
		Name = InName;
	}
	void SetSequence(UAnimSequence* InSequence)
	{
		AnimSequence = InSequence;
	}
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
public:
	FString Name;
	UAnimSequence* AnimSequence = nullptr;

	float Speed = 1.0f;
	float StartTime = 0.0f;
	bool bLoop = true;

	
};