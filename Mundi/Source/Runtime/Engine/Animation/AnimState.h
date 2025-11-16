#pragma once
#include "Source/Runtime/Core/Object/Object.h"

class UAnimSequence;

class UAnimState : public UObject
{
	DECLARE_CLASS(UAnimState, UObject)
public:
	UAnimState() = default;
	virtual ~UAnimState() = default;
public:
	FString Name;
	UAnimSequence* AnimSequence = nullptr;

	float Speed = 1.0f;
	float StartTime = 0.0f;
	bool bLoop = true;

	
};