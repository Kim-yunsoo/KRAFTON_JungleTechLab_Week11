#pragma once
#include "Source/Runtime/Core/Object/Object.h"

class UAnimState;

class UAnimTransition : public UObject
{
	DECLARE_CLASS(UAnimTransition, UObject)

public:
	UAnimTransition() = default;
	virtual ~UAnimTransition() = default;
	bool CanEnter() const { return Condition(); }

	void SetRoot(UAnimState* InFrom, UAnimState* InTo)
	{
		From = InFrom;
		To = InTo;
	}

	void SetBlendTime(const float InBlendTime)
	{
		BlendTime = InBlendTime;
	}
	void SetCondition(const std::function<bool()> InCondition)
	{
		Condition = InCondition;
	}
public:
	UAnimState* From;
	UAnimState* To;
	float BlendTime = 0.2f;

	std::function<bool()> Condition;

};