#pragma once
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "UMyAnimInstance.generated.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"

class UMyAnimInstance : public UAnimInstance
{
	GENERATED_REFLECTION_BODY()

public:
	UMyAnimInstance() = default;
	virtual ~UMyAnimInstance() = default;
public:

	UPROPERTY(EditAnywhere, Category = "AnimInstance")
	UAnimSequence* AnimA;

	UPROPERTY(EditAnywhere, Category = "AnimInstance")
	UAnimSequence* AnimB;

	UPROPERTY(EditAnywhere, Category = "AnimInstance", Range = "0.0, 1.0")
	float BlendAlpha = 0.0f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};