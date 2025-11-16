#include "pch.h"
#include "SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"
#include "Source/Runtime/Engine/Animation/MyAnimInstance.h"
#include <functional>
USkeletalMeshComponent::USkeletalMeshComponent()
{
    // 테스트용 기본 메시 설정
    SetSkeletalMesh(GDataDir + "/Test.fbx"); 
}
USkeletalMeshComponent::~USkeletalMeshComponent()
{
    if (AnimInstance)
    {
        DeleteObject(AnimInstance);
        AnimInstance = nullptr;
    }
}

void USkeletalMeshComponent::BeginPlay()
{
    AnimInstance = NewObject<UAnimInstance>();
    AnimInstance->SetOwner(this);

    UAnimStateMachine& StateMachine = AnimInstance->GetStateMachine();
    UAnimState* IdleState = NewObject<UAnimState>();
    IdleState->Name = "Idle";
    IdleState->AnimSequence = RESOURCE.Get<UAnimSequence>("Data/Animations/Breathing Idle.fbx");
    UAnimState* MoveState = NewObject<UAnimState>();
    MoveState->Name = "Move";
    MoveState->AnimSequence = RESOURCE.Get<UAnimSequence>("Data/Animations/Standard Walk.fbx");
    StateMachine.AddState(IdleState);
    StateMachine.AddState(MoveState);
    StateMachine.StartStateMachine(IdleState);

    UAnimTransition* IdleToMoveTransition = NewObject<UAnimTransition>();
    IdleToMoveTransition->From = IdleState;
    IdleToMoveTransition->To = MoveState;
    IdleToMoveTransition->Condition = [this]()->bool {return bMove; };
    UAnimTransition* MoveToIdleTransition = NewObject<UAnimTransition>();
    MoveToIdleTransition->From = MoveState;
    MoveToIdleTransition->To = IdleState;
    MoveToIdleTransition->Condition = [this]()->bool {return !bMove; };
    StateMachine.AddTransition(IdleToMoveTransition);
    StateMachine.AddTransition(MoveToIdleTransition);

    AnimInstance->SetPlay(true);
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (AnimInstance)
    {
        AnimInstance->Tick(DeltaTime);
    }
}

void USkeletalMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
    Super::SetSkeletalMesh(PathFileName);

    if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
    {
        const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
        const int32 NumBones = Skeleton.Bones.Num();

        CurrentLocalSpacePose.SetNum(NumBones);
        CurrentComponentSpacePose.SetNum(NumBones);
        TempFinalSkinningMatrices.SetNum(NumBones);
        TempFinalSkinningNormalMatrices.SetNum(NumBones);

        for (int32 i = 0; i < NumBones; ++i)
        {
            const FBone& ThisBone = Skeleton.Bones[i];
            const int32 ParentIndex = ThisBone.ParentIndex;
            FMatrix LocalBindMatrix;

            if (ParentIndex == -1) // 루트 본
            {
                LocalBindMatrix = ThisBone.BindPose;
            }
            else // 자식 본
            {
                const FMatrix& ParentInverseBindPose = Skeleton.Bones[ParentIndex].InverseBindPose;
                LocalBindMatrix = ThisBone.BindPose * ParentInverseBindPose;
            }
            // 계산된 로컬 행렬을 로컬 트랜스폼으로 변환
            CurrentLocalSpacePose[i] = FTransform(LocalBindMatrix); 
        }
        
        ForceRecomputePose(); 
    }
    else
    {
        // 메시 로드 실패 시 버퍼 비우기
        CurrentLocalSpacePose.Empty();
        CurrentComponentSpacePose.Empty();
        TempFinalSkinningMatrices.Empty();
        TempFinalSkinningNormalMatrices.Empty();
    }
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        CurrentLocalSpacePose[BoneIndex] = NewLocalTransform;
        ForceRecomputePose();
    }
}

void USkeletalMeshComponent::SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform)
{
    if (BoneIndex < 0 || BoneIndex >= CurrentLocalSpacePose.Num())
        return;

    const int32 ParentIndex = SkeletalMesh->GetSkeleton()->Bones[BoneIndex].ParentIndex;

    const FTransform& ParentWorldTransform = GetBoneWorldTransform(ParentIndex);
    FTransform DesiredLocal = ParentWorldTransform.GetRelativeTransform(NewWorldTransform);

    SetBoneLocalTransform(BoneIndex, DesiredLocal);
}

void USkeletalMeshComponent::SetPose(const FPoseContext& Pose)
{
    if (Pose.Pose.Num() == CurrentLocalSpacePose.Num())
    {
        CurrentLocalSpacePose = Pose.Pose;
        ForceRecomputePose();
    }
}
const TArray<FTransform>& USkeletalMeshComponent::GetPose() const
{
    return CurrentLocalSpacePose;
}

FTransform USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        return CurrentLocalSpacePose[BoneIndex];
    }
    return FTransform();
}

FTransform USkeletalMeshComponent::GetBoneWorldTransform(int32 BoneIndex)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex && BoneIndex >= 0)
    {
        // 뼈의 컴포넌트 공간 트랜스폼 * 컴포넌트의 월드 트랜스폼
        return GetWorldTransform().GetWorldTransform(CurrentComponentSpacePose[BoneIndex]);
    }
    return GetWorldTransform(); // 실패 시 컴포넌트 위치 반환
}

void USkeletalMeshComponent::ForceRecomputePose()
{
    if (!SkeletalMesh) { return; } 

    // LocalSpace -> ComponentSpace 계산
    UpdateComponentSpaceTransforms();
    // ComponentSpace -> Final Skinning Matrices 계산
    UpdateFinalSkinningMatrices();
    UpdateSkinningMatrices(TempFinalSkinningMatrices, TempFinalSkinningNormalMatrices);
    PerformSkinning();
}

void USkeletalMeshComponent::UpdateComponentSpaceTransforms()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FTransform& LocalTransform = CurrentLocalSpacePose[BoneIndex];
        const int32 ParentIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        if (ParentIndex == -1) // 루트 본
        {
            CurrentComponentSpacePose[BoneIndex] = LocalTransform;
        }
        else // 자식 본
        {
            const FTransform& ParentComponentTransform = CurrentComponentSpacePose[ParentIndex];
            CurrentComponentSpacePose[BoneIndex] = ParentComponentTransform.GetWorldTransform(LocalTransform);
        }
    }
}

void USkeletalMeshComponent::UpdateFinalSkinningMatrices()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FMatrix& InvBindPose = Skeleton.Bones[BoneIndex].InverseBindPose;
        const FMatrix ComponentPoseMatrix = CurrentComponentSpacePose[BoneIndex].ToMatrix();

        TempFinalSkinningMatrices[BoneIndex] = InvBindPose * ComponentPoseMatrix;
        TempFinalSkinningNormalMatrices[BoneIndex] = TempFinalSkinningMatrices[BoneIndex].Inverse().Transpose();
    }
}
void USkeletalMeshComponent::PlayAnimation(UAnimSequence* InAnimSequence, bool bLoop)
{
    if (!InAnimSequence)
    {
        return;
    }

    UAnimSingleNodeInstance* SingleNode = nullptr;
    if (AnimInstance == nullptr)
    {
        SingleNode = NewObject<UAnimSingleNodeInstance>();
        AnimInstance = SingleNode;
        AnimInstance->SetOwner(this);
    }
    else
    {
        if (SingleNode = Cast<UAnimSingleNodeInstance>(AnimInstance))
        {
        }
        else
        {
            DeleteObject(AnimInstance);
            SingleNode = NewObject<UAnimSingleNodeInstance>();
            AnimInstance = SingleNode;
            AnimInstance->SetOwner(this);
        }
    }

    SingleNode->SetAnimSequence(InAnimSequence, bLoop);
}