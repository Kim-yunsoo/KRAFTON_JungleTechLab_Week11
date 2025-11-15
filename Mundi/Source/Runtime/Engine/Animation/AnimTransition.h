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

public:
	UAnimState* From;
	UAnimState* To;

	std::is_function<bool()> Condition;

};