#pragma once
#include "Source/Runtime/Core/Math/Vector.h"

class UAnimInstance;
class UAnimSequence;

struct FPoseContext
{
    TArray<FTransform> Pose;
    UAnimInstance* AnimInstance;
    FPoseContext(UAnimInstance* InAnimInstance);
    void SetPose(UAnimSequence* AnimSequence, const float Time);

    static void BlendTwoPoses(const FPoseContext& PoseA, const FPoseContext& PoseB, const float BlendAlpha, FPoseContext& OutPose);
};