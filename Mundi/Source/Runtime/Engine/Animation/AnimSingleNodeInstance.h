#pragma once
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
public:
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() = default;

protected:
	void NativeUpdateAnimation(float DeltaSeconds) override;

};