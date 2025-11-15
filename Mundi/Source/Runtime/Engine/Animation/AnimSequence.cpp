#include "pch.h"
#include "AnimSequence.h"
#include "AnimDataModel.h"
#include "JsonSerializer.h"

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
    static TArray<FBoneAnimationTrack> EmptyTracks;
    return DataModel ? DataModel->GetBoneTracks() : EmptyTracks;
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
    static FFrameRate DefaultRate;
    return DataModel ? DataModel->GetFrameRate() : DefaultRate;
}

void UAnimSequence::SyncDerivedMetadata()
{
    // DataModel이 존재하면 길이를 계산하고, 없으면 0초로 맞춘다.
    DataModel ? SetSequenceLength(DataModel->GetPlayLengthSeconds()) : SetSequenceLength(0.f);
}

// AnimSequence.cpp 내부에서만 사용되므로 namespace로 범위를 한정합니다.
namespace
{
    // JSON 직렬화/역직렬화를 돕는 로컬 유틸 함수들.
    JSON SerializeVectorKeys(const TArray<FVector>& Keys)
    {
        JSON ArrayJson = JSON::Make(JSON::Class::Array);
        for (const FVector& Key : Keys)
        {
            JSON KeyJson = JSON::Make(JSON::Class::Array);
            KeyJson.append(Key.X);
            KeyJson.append(Key.Y);
            KeyJson.append(Key.Z);
            ArrayJson.append(KeyJson);
        }
        return ArrayJson;
    }

    JSON SerializeQuatKeys(const TArray<FQuat>& Keys)
    {
        JSON ArrayJson = JSON::Make(JSON::Class::Array);
        for (const FQuat& Key : Keys)
        {
            JSON KeyJson = JSON::Make(JSON::Class::Array);
            KeyJson.append(Key.X);
            KeyJson.append(Key.Y);
            KeyJson.append(Key.Z);
            KeyJson.append(Key.W);
            ArrayJson.append(KeyJson);
        }
        return ArrayJson;
    }

    void DeserializeVectorKeys(const JSON& JsonArray, TArray<FVector>& OutKeys)
    {
        OutKeys.clear();
        if (JsonArray.JSONType() != JSON::Class::Array)
        {
            return;
        }

        OutKeys.reserve(static_cast<int32>(JsonArray.size()));
        for (size_t Idx = 0; Idx < JsonArray.size(); ++Idx)
        {
            const JSON& KeyJson = JsonArray.at(static_cast<unsigned>(Idx));
            if (KeyJson.JSONType() != JSON::Class::Array || KeyJson.size() < 3)
            {
                continue;
            }

            FVector Key;
            Key.X = static_cast<float>(KeyJson.at(0).ToFloat());
            Key.Y = static_cast<float>(KeyJson.at(1).ToFloat());
            Key.Z = static_cast<float>(KeyJson.at(2).ToFloat());
            OutKeys.Add(Key);
        }
    }

    void DeserializeQuatKeys(const JSON& JsonArray, TArray<FQuat>& OutKeys)
    {
        OutKeys.clear();
        if (JsonArray.JSONType() != JSON::Class::Array)
        {
            return;
        }

        OutKeys.reserve(static_cast<int32>(JsonArray.size()));
        for (size_t Idx = 0; Idx < JsonArray.size(); ++Idx)
        {
            const JSON& KeyJson = JsonArray.at(static_cast<unsigned>(Idx));
            if (KeyJson.JSONType() != JSON::Class::Array || KeyJson.size() < 4)
            {
                continue;
            }

            FQuat Key;
            Key.X = static_cast<float>(KeyJson.at(0).ToFloat());
            Key.Y = static_cast<float>(KeyJson.at(1).ToFloat());
            Key.Z = static_cast<float>(KeyJson.at(2).ToFloat());
            Key.W = static_cast<float>(KeyJson.at(3).ToFloat());
            OutKeys.Add(Key);
        }
    }

    JSON SerializeBoneTrack(const FBoneAnimationTrack& Track)
    {
        JSON TrackJson = JSON::Make(JSON::Class::Object);
        TrackJson["BoneName"] = Track.BoneName.ToString().c_str();
        TrackJson["BoneIndex"] = Track.BoneIndex;
        TrackJson["PosKeys"] = SerializeVectorKeys(Track.InternalTrack.PosKeys);
        TrackJson["RotKeys"] = SerializeQuatKeys(Track.InternalTrack.RotKeys);
        TrackJson["ScaleKeys"] = SerializeVectorKeys(Track.InternalTrack.ScaleKeys);
        return TrackJson;
    }

    void DeserializeBoneTracks(const JSON& TracksJson, TArray<FBoneAnimationTrack>& OutTracks)
    {
        OutTracks.clear();
        if (TracksJson.JSONType() != JSON::Class::Array)
        {
            return;
        }

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

            if (TrackJson.hasKey("PosKeys"))
            {
                DeserializeVectorKeys(TrackJson.at("PosKeys"), Track.InternalTrack.PosKeys);
            }
            if (TrackJson.hasKey("RotKeys"))
            {
                DeserializeQuatKeys(TrackJson.at("RotKeys"), Track.InternalTrack.RotKeys);
            }
            if (TrackJson.hasKey("ScaleKeys"))
            {
                DeserializeVectorKeys(TrackJson.at("ScaleKeys"), Track.InternalTrack.ScaleKeys);
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
    else
    {
        if (!DataModel) { return; }

        // DataModel 내용을 JSON에 기록해 디스크로 보낸다.
        JSON ModelJson = JSON::Make(JSON::Class::Object);
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

        InOutHandle["DataModel"] = ModelJson;
    }
}
