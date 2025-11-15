#pragma once
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "UAnimBlendInstance.generated.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"

class UAnimBlendInstance : public UAnimInstance
{
	GENERATED_REFLECTION_BODY()

public:
	UAnimBlendInstance() = default;
	virtual ~UAnimBlendInstance() = default;
	void SetBlendAnimation(UAnimSequence* SequenceA, UAnimSequence* SequenceB);
	void SetBlendAlpha(const float InBlendAlpha);

public:

	UPROPERTY(EditAnywhere, Category = "AnimBlendInstance")
	UAnimSequence* AnimA;

	UPROPERTY(EditAnywhere, Category = "AnimBlendInstance")
	UAnimSequence* AnimB;

	UPROPERTY(EditAnywhere, Category = "AnimBlendInstance", Range = "0.0, 1.0")
	float BlendAlpha = 0.0f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

};