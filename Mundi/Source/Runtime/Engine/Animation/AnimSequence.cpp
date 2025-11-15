#include "pch.h"
#include "AnimSequence.h"
#include "AnimDataModel.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UAnimSequence)

void UAnimSequence::SetDataModel(std::unique_ptr<UAnimDataModel> InDataModel)
{
    // 새로운 데이터 모델을 받고 시퀀스 메타데이터를 즉시 맞춘다.
    DataModel = std::move(InDataModel);
    SyncDerivedMetadata();
}

void UAnimSequence::ResetDataModel()
{
    // 모델 소유권을 비우면 재생 길이 역시 0초로 리셋된다.
    DataModel.reset();
    SyncDerivedMetadata();
}

const TArray<FBoneAnimationTrack>& UAnimSequence::GetBoneTracks() const
{
    if (DataModel)
    {
        return DataModel->GetBoneTracks();
    }

    // DataModel이 없을 때 빈 배열 반환 (static 대신 지역 const 사용)
    static const TArray<FBoneAnimationTrack> EmptyTracks;
    return EmptyTracks;
}

const FBoneAnimationTrack* UAnimSequence::FindTrackByBoneName(const FName& BoneName) const
{
    return DataModel ? DataModel->FindTrackByBoneName(BoneName) : nullptr;
}

const FBoneAnimationTrack* UAnimSequence::FindTrackByBoneIndex(int32 BoneIndex) const
{
    return DataModel ? DataModel->FindTrackByBoneIndex(BoneIndex) : nullptr;
}

int32 UAnimSequence::GetNumberOfFrames() const
{
    return DataModel ? DataModel->GetNumberOfFrames() : 0;
}

int32 UAnimSequence::GetNumberOfKeys() const
{
    return DataModel ? DataModel->GetNumberOfKeys() : 0;
}

const FFrameRate& UAnimSequence::GetFrameRateStruct() const
{
    if (DataModel)
    {
        return DataModel->GetFrameRate();
    }

    // DataModel이 없을 때 기본 프레임레이트 반환 (static const로 불변성 보장)
    static const FFrameRate DefaultRate{};
    return DefaultRate;
}

void UAnimSequence::SyncDerivedMetadata()
{
    // DataModel이 존재하면 길이를 계산하고, 없으면 0초로 맞춘다.
    DataModel ? SetSequenceLength(DataModel->GetPlayLengthSeconds()) : SetSequenceLength(0.f);
}

// AnimSequence.cpp 내부에서만 사용되므로 namespace로 범위를 한정합니다.
namespace
{
    // SerializePrimitiveArray (Object.h)를 활용한 본 트랙 직렬화 헬퍼
    JSON SerializeBoneTrack(const FBoneAnimationTrack& Track)
    {
        JSON TrackJson = JSON::Make(JSON::Class::Object);
        TrackJson["BoneName"] = Track.BoneName.ToString().c_str();
        TrackJson["BoneIndex"] = Track.BoneIndex;

        // SerializePrimitiveArray 활용
        JSON PosKeysJson = JSON::Make(JSON::Class::Array);
        JSON RotKeysJson = JSON::Make(JSON::Class::Array);
        JSON ScaleKeysJson = JSON::Make(JSON::Class::Array);
        JSON KeyTimesJson = JSON::Make(JSON::Class::Array);

        SerializePrimitiveArray(const_cast<TArray<FVector>*>(&Track.InternalTrack.PosKeys), false, PosKeysJson);
        SerializePrimitiveArray(const_cast<TArray<FQuat>*>(&Track.InternalTrack.RotKeys), false, RotKeysJson);
        SerializePrimitiveArray(const_cast<TArray<FVector>*>(&Track.InternalTrack.ScaleKeys), false, ScaleKeysJson);
        SerializePrimitiveArray(const_cast<TArray<float>*>(&Track.InternalTrack.KeyTimes), false, KeyTimesJson);

        TrackJson["PosKeys"] = PosKeysJson;
        TrackJson["RotKeys"] = RotKeysJson;
        TrackJson["ScaleKeys"] = ScaleKeysJson;
        TrackJson["KeyTimes"] = KeyTimesJson;

        return TrackJson;
    }

    void DeserializeBoneTracks(const JSON& TracksJson, TArray<FBoneAnimationTrack>& OutTracks)
    {
        OutTracks.clear();
        if (TracksJson.JSONType() != JSON::Class::Array) { return; }

        OutTracks.reserve(static_cast<int32>(TracksJson.size()));
        for (size_t Idx = 0; Idx < TracksJson.size(); ++Idx)
        {
            const JSON& TrackJson = TracksJson.at(static_cast<unsigned>(Idx));
            if (TrackJson.JSONType() != JSON::Class::Object) { continue; }

            FBoneAnimationTrack Track;

            FString BoneNameString;
            if (FJsonSerializer::ReadString(TrackJson, "BoneName", BoneNameString, "", false))
            {
                Track.BoneName = FName(BoneNameString);
            }
            FJsonSerializer::ReadInt32(TrackJson, "BoneIndex", Track.BoneIndex, -1, false);

            // SerializePrimitiveArray 활용
            if (TrackJson.hasKey("PosKeys"))
            {
                JSON PosKeysJson = TrackJson.at("PosKeys");
                SerializePrimitiveArray(&Track.InternalTrack.PosKeys, true, PosKeysJson);
            }
            if (TrackJson.hasKey("RotKeys"))
            {
                JSON RotKeysJson = TrackJson.at("RotKeys");
                SerializePrimitiveArray(&Track.InternalTrack.RotKeys, true, RotKeysJson);
            }
            if (TrackJson.hasKey("ScaleKeys"))
            {
                JSON ScaleKeysJson = TrackJson.at("ScaleKeys");
                SerializePrimitiveArray(&Track.InternalTrack.ScaleKeys, true, ScaleKeysJson);
            }
            if (TrackJson.hasKey("KeyTimes"))
            {
                JSON KeyTimesJson = TrackJson.at("KeyTimes");
                SerializePrimitiveArray(&Track.InternalTrack.KeyTimes, true, KeyTimesJson);
            }

            OutTracks.Add(Track);
        }
    }
}

void UAnimSequence::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        if (InOutHandle.hasKey("DataModel"))
        {
            const JSON& ModelJson = InOutHandle.at("DataModel");
            auto NewModel = std::make_unique<UAnimDataModel>();

            // JSON에 저장된 프레임레이트/트랙 정보를 역직렬화한다.
            FFrameRate Rate;
            FJsonSerializer::ReadInt32(ModelJson, "FrameRateNumerator", Rate.Numerator, Rate.Numerator, false);
            FJsonSerializer::ReadInt32(ModelJson, "FrameRateDenominator", Rate.Denominator, Rate.Denominator, false);
            NewModel->SetFrameRate(Rate);

            if (ModelJson.hasKey("Tracks"))
            {
                TArray<FBoneAnimationTrack> Tracks;
                DeserializeBoneTracks(ModelJson.at("Tracks"), Tracks);
                NewModel->SetBoneTracks(std::move(Tracks));
            }
            else
            {
                NewModel->Reset();
            }

            DataModel = std::move(NewModel);
        }
        else
        {
            DataModel.reset();
        }

        SyncDerivedMetadata();
    }
    else // Saving
    {
        // DataModel이 없어도 일관성을 위해 빈 구조를 저장
        JSON ModelJson = JSON::Make(JSON::Class::Object);

        if (DataModel)
        {
            // DataModel 내용을 JSON에 기록해 디스크로 보낸다.
            ModelJson["FrameRateNumerator"] = DataModel->GetFrameRate().Numerator;
            ModelJson["FrameRateDenominator"] = DataModel->GetFrameRate().Denominator;
            ModelJson["NumberOfFrames"] = DataModel->GetNumberOfFrames();
            ModelJson["NumberOfKeys"] = DataModel->GetNumberOfKeys();
            ModelJson["PlayLength"] = DataModel->GetPlayLengthSeconds();

            JSON TrackArray = JSON::Make(JSON::Class::Array);
            for (const FBoneAnimationTrack& Track : DataModel->GetBoneTracks())
            {
                TrackArray.append(SerializeBoneTrack(Track));
            }
            ModelJson["Tracks"] = TrackArray;
        }
        else
        {
            // DataModel이 없을 때 기본값으로 빈 구조 저장
            ModelJson["FrameRateNumerator"] = 30;
            ModelJson["FrameRateDenominator"] = 1;
            ModelJson["NumberOfFrames"] = 0;
            ModelJson["NumberOfKeys"] = 0;
            ModelJson["PlayLength"] = 0.0f;
            ModelJson["Tracks"] = JSON::Make(JSON::Class::Array);
        }

        InOutHandle["DataModel"] = ModelJson;
    }
}
