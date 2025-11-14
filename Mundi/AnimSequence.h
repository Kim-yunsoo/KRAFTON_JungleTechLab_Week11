#pragma once
#include "AnimSequenceBase.h"

class UAnimSequence : public UAnimSequenceBase
{
public:
    DECLARE_CLASS(UAnimSequence, UAnimSequenceBase);

    UAnimSequence() {}
    virtual ~UAnimSequence()
    {
        if (DataModel)
            delete DataModel;
    }

    // 실제 애니메이션 데이터
    UAnimDataModel* DataModel = nullptr;

    virtual float GetPlayLength() const override
    {
        return DataModel ? DataModel->PlayLength : 0.f;
    }

    const TArray<FBoneAnimationTrack>& GetBoneTracks() const
    {
        return DataModel->BoneAnimationTracks;
    }

    int32 GetNumFrames() const
    {
        return DataModel ? DataModel->NumberOfFrames : 0;
    }

    float GetFrameRate() const
    {
        return DataModel ? DataModel->FrameRate.AsDecimal() : 0.f;
    }
};

