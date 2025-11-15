#include "pch.h"
#include "Source/Runtime/Engine/Animation/AnimationTypes.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"

FPoseContext::FPoseContext(UAnimInstance* InAnimInstance) : AnimInstance(InAnimInstance)
{
    Pose.resize(InAnimInstance->GetOwner()->GetSkeletalMesh()->GetSkeleton()->Bones.Num());
}
void FPoseContext::SetPose(UAnimSequence* AnimSequence, const float Time)
{
    int BoneCount = Pose.Num();
    float FrameRate = AnimSequence->GetFrameRate();
    for (const FBoneAnimationTrack& Track : AnimSequence->GetBoneTracks())
    {
        if (BoneCount > Track.BoneIndex && Track.BoneIndex >= 0)
        {
            Pose[Track.BoneIndex] = Track.InternalTrack.GetTransform(FrameRate, Time);
        }
    }
}

void FPoseContext::BlendTwoPoses(const FPoseContext& PoseA, const FPoseContext& PoseB, const float BlendAlpha, FPoseContext& OutPose)
{
    uint32 PoseNum = PoseA.Pose.Num();
    if (PoseNum != PoseB.Pose.Num())
    {
        return;
    }
    for (int i = 0; i < PoseNum; i++)
    {
        OutPose.Pose[i] = FTransform::Lerp(PoseA.Pose[i], PoseB.Pose[i], BlendAlpha);
    }
}
