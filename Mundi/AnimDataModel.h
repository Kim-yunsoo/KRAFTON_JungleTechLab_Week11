#pragma once
#include "Object.h"

class UAnimDataModel : public UObject
{
public:
    DECLARE_CLASS(UAnimDataModel, UObject)

    UAnimDataModel() = default;
    virtual ~UAnimDataModel() override = default;

    // ▣ 실제 애니메이션 Bone Track 데이터
    TArray<FBoneAnimationTrack> BoneAnimationTracks;

    // ▣ 전체 재생 시간(초)
    float PlayLength = 0.f;

    // ▣ Helper
    int32 GetNumTracks() const
    {
        return static_cast<int32>(BoneAnimationTracks.size());
    }

    // Bone 이름으로 Track을 찾고 싶을 때
    const FBoneAnimationTrack* FindTrack(const FName& InOtherBoneName) const
    {
        for (const auto& Track : BoneAnimationTracks)
        {
            if (Track.BoneName == InOtherBoneName)
                return &Track;
        }
        return nullptr;
    }
};

