#include "pch.h"
#include "SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"
#include "Source/Runtime/Engine/Animation/MyAnimInstance.h"
#include "Source/Runtime/Core/Object/Actor.h"
#include <functional>

USkeletalMeshComponent::USkeletalMeshComponent()
{
    // 테스트용 기본 메시 설정
    SetSkeletalMesh(GDataDir + "/Test.fbx"); 
    AnimInstance = NewObject<UAnimInstance>();
    AnimInstance->SetOwner(this);
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
    //AnimInstance->AddState("Idle", RESOURCE.Get<UAnimSequence>("Data/Animations/Breathing Idle.fbx"));
    //AnimInstance->AddState("Move", RESOURCE.Get<UAnimSequence>("Data/Animations/Standard Walk.fbx"));
    //AnimInstance->SetStartState("Idle");
    //AnimInstance->AddTransition("Idle", "Move")->SetCondition([this]()->bool {return bMove; });
    //AnimInstance->AddTransition("Move", "Idle")->SetCondition([this]()->bool {return !bMove; });

    UAnimStateMachine& StateMachine = AnimInstance->GetStateMachine();
    UAnimState* IdleState = NewObject<UAnimState>();
    IdleState->Name = "Idle";
    IdleState->AnimSequence = RESOURCE.Get<UAnimSequence>("Data/Animations/Breathing Idle.fbx");
    UAnimState* MoveState = NewObject<UAnimState>();
    MoveState->Name = "Move";
    MoveState->AnimSequence = RESOURCE.Get<UAnimSequence>("Data/Animations/Standard Walk.fbx");
    // Note: test AnimNotify injection removed; use Sequence Viewer UI to author notifies
    StateMachine.AddState(IdleState->AnimSequence->GetName(), IdleState->AnimSequence);
    StateMachine.AddState(MoveState->AnimSequence->GetName(), MoveState->AnimSequence);
    StateMachine.StartStateMachine(IdleState);

    UAnimTransition* IdleToMoveTransition = NewObject<UAnimTransition>();
    IdleToMoveTransition->From = IdleState;
    IdleToMoveTransition->To = MoveState;
    IdleToMoveTransition->Condition = [this]()->bool {return bMove; };
    UAnimTransition* MoveToIdleTransition = NewObject<UAnimTransition>();
    MoveToIdleTransition->From = MoveState;
    MoveToIdleTransition->To = IdleState;
    MoveToIdleTransition->Condition = [this]()->bool {return !bMove; };
    //StateMachine.AddTransition(IdleState->AnimSequence->GetName(),IdleToMoveTransition);
    //StateMachine.AddTransition(MoveState->AnimSequence->GetName(),MoveToIdleTransition);
    StateMachine.AddTransition(IdleState->AnimSequence->GetName(), MoveState->AnimSequence->GetName());
    AnimInstance->Play();
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (AnimInstance)
    {
        AnimInstance->SetSpeed(TestSpeed);
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

void USkeletalMeshComponent::SetLocalSpacePose(const TArray<FTransform>& InPose)
{
    if (InPose.Num() == CurrentLocalSpacePose.Num())
    {
        CurrentLocalSpacePose = InPose;
        ForceRecomputePose();
    }
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

void USkeletalMeshComponent::MarkBoneAsManuallyEdited(int32 BoneIndex)
{
    if (BoneIndex >= 0 && BoneIndex < CurrentLocalSpacePose.Num())
    {
        // 중복 방지
        if (!ManuallyEditedBones.Contains(BoneIndex))
        {
            ManuallyEditedBones.Add(BoneIndex);
        }
    }
}

bool USkeletalMeshComponent::IsBoneManuallyEdited(int32 BoneIndex) const
{
    return ManuallyEditedBones.Contains(BoneIndex);
}

void USkeletalMeshComponent::ClearManuallyEditedBones()
{
    ManuallyEditedBones.Empty();
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

void USkeletalMeshComponent::PlayAnimation(const FString& AnimPath, bool bLoop)
{
    UAnimSequence* AnimSequence = RESOURCE.Get<UAnimSequence>(AnimPath);
    if (!AnimSequence)
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

    SingleNode->SetAnimSequence(AnimSequence, bLoop);
}

void USkeletalMeshComponent::HandleAnimNotify(const FAnimNotifyEvent& Notify)
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->HandleAnimNotify(Notify);
    }
    //SingleNode->SetAnimSequence(AnimSequence, bLoop);
}

void USkeletalMeshComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
    AnimInstance = NewObject<UAnimInstance>();
    AnimInstance->SetOwner(this);
}



void USkeletalMeshComponent::AddState(const FString& InName, const FString& AnimPath)
{
    if (AnimInstance)
    {
        AnimInstance->AddState(InName, AnimPath);
    }
}

void USkeletalMeshComponent::AddTransition(const FString& StartStateName, const FString& EndStateName, const float InBlendTime, std::function<bool()> func)
{ 
    if (AnimInstance)
    {
        UAnimTransition* Transition = AnimInstance->AddTransition(StartStateName, EndStateName);
        Transition->SetBlendTime(InBlendTime);
        Transition->SetCondition(func);
    }
}

void USkeletalMeshComponent::SetStartState(const FString& StartStateName)
{ 
    if (AnimInstance)
    {
        AnimInstance->SetStartState(StartStateName);
    }
}

void USkeletalMeshComponent::SetSpeed(const float InSpeed)
{ 
    if (AnimInstance)
    {
        AnimInstance->SetSpeed(InSpeed);
    }
}

void USkeletalMeshComponent::Play()
{ 
    if (AnimInstance)
    {
        AnimInstance->Play();
    }
}

void USkeletalMeshComponent::Pause()
{ 
    if (AnimInstance)
    {
        AnimInstance->Pause();
    }
}

void USkeletalMeshComponent::Replay()
{
    if (AnimInstance)
    {
        AnimInstance->Replay();
    }
}
