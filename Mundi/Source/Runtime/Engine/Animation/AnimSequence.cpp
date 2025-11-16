#include "pch.h"
#include "AnimSequence.h"
#include "AnimDataModel.h"
#include "JsonSerializer.h"
#include "PathUtils.h"

#include <filesystem>

IMPLEMENT_CLASS(UAnimSequence)

void UAnimSequence::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    (void)InDevice;

    FString NormalizedPath = NormalizePath(InFilePath);
    JSON AnimJson;
    FWideString WidePath = UTF8ToWide(NormalizedPath);

    if (!FJsonSerializer::LoadJsonFromFile(AnimJson, WidePath))
    {
        UE_LOG("[error] UAnimSequence::Load: Failed to read %s", NormalizedPath.c_str());
        SetFilePath(NormalizedPath);
        ResetDataModel();
        ClearNotifies();
        SetLastModifiedTime(std::filesystem::file_time_type::clock::now());
        return;
    }

    Serialize(true, AnimJson);

    if (GetFilePath().empty())
    {
        SetFilePath(NormalizedPath);
    }

    try
    {
        std::filesystem::path FsPath(WidePath);
        SetLastModifiedTime(std::filesystem::last_write_time(FsPath));
    }
    catch (...)
    {
        SetLastModifiedTime(std::filesystem::file_time_type::clock::now());
    }
}

void UAnimSequence::SetDataModel(std::unique_ptr<UAnimDataModel> InDataModel)
{
    DataModel = std::move(InDataModel);
    SyncDerivedMetadata();
}

void UAnimSequence::ResetDataModel()
{
    DataModel.reset();
    SyncDerivedMetadata();
}

const TArray<FBoneAnimationTrack>& UAnimSequence::GetBoneTracks() const
{
    if (DataModel)
    {
        return DataModel->GetBoneTracks();
    }

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

const TArray<FCurveTrack>& UAnimSequence::GetCurveTracks() const
{
    if (DataModel)
    {
        return DataModel->GetCurveTracks();
    }
    static const TArray<FCurveTrack> Empty;
    return Empty;
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

    static const FFrameRate DefaultRate{};
    return DefaultRate;
}

void UAnimSequence::SyncDerivedMetadata()
{
    DataModel ? SetSequenceLength(DataModel->GetPlayLengthSeconds()) : SetSequenceLength(0.f);
}

namespace
{
    // === Bone Tracks ===
    JSON SerializeBoneTrack(const FBoneAnimationTrack& Track)
    {
        JSON TrackJson = JSON::Make(JSON::Class::Object);
        TrackJson["BoneName"] = Track.BoneName.ToString().c_str();
        TrackJson["BoneIndex"] = Track.BoneIndex;

        // SerializePrimitiveArray Ȱ��
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

            // SerializePrimitiveArray Ȱ��
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

    // === Curve Tracks ===
    JSON SerializeCurveTrack(const FCurveTrack& Curve)
    {
        JSON CurveJson = JSON::Make(JSON::Class::Object);
        CurveJson["Name"] = Curve.CurveName.ToString().c_str();

        JSON KeysJson = JSON::Make(JSON::Class::Array);
        for (const FFloatCurveKey& K : Curve.Keys)
        {
            JSON KJson = JSON::Make(JSON::Class::Object);
            KJson["Time"] = K.Time;
            KJson["Value"] = K.Value;
            KeysJson.append(KJson);
        }
        CurveJson["Keys"] = KeysJson;
        return CurveJson;
    }

    void DeserializeCurveTracks(const JSON& CurvesJson, TArray<FCurveTrack>& OutCurves)
    {
        OutCurves.clear();
        if (CurvesJson.JSONType() != JSON::Class::Array) { return; }

        OutCurves.reserve(static_cast<int32>(CurvesJson.size()));
        for (size_t Idx = 0; Idx < CurvesJson.size(); ++Idx)
        {
            const JSON& CJson = CurvesJson.at(static_cast<unsigned>(Idx));
            if (CJson.JSONType() != JSON::Class::Object) { continue; }

            FCurveTrack Curve;
            FString NameString;
            if (FJsonSerializer::ReadString(CJson, "Name", NameString, "", false))
            {
                Curve.CurveName = FName(NameString);
            }

            if (CJson.hasKey("Keys"))
            {
                const JSON& KeysJson = CJson.at("Keys");
                if (KeysJson.JSONType() == JSON::Class::Array)
                {
                    Curve.Keys.reserve(static_cast<int32>(KeysJson.size()));
                    for (size_t k = 0; k < KeysJson.size(); ++k)
                    {
                        const JSON& KJson = KeysJson.at(static_cast<unsigned>(k));
                        if (KJson.JSONType() != JSON::Class::Object) { continue; }
                        FFloatCurveKey Key;
                        FJsonSerializer::ReadFloat(KJson, "Time", Key.Time, Key.Time, false);
                        FJsonSerializer::ReadFloat(KJson, "Value", Key.Value, Key.Value, false);
                        Curve.Keys.Add(Key);
                    }
                }
            }

            OutCurves.Add(Curve);
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

            if (ModelJson.hasKey("Curves"))
            {
                TArray<FCurveTrack> Curves;
                DeserializeCurveTracks(ModelJson.at("Curves"), Curves);
                NewModel->SetCurveTracks(std::move(Curves));
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
        JSON ModelJson = JSON::Make(JSON::Class::Object);

        if (DataModel)
        {
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

            JSON CurvesArray = JSON::Make(JSON::Class::Array);
            for (const FCurveTrack& Curve : DataModel->GetCurveTracks())
            {
                CurvesArray.append(SerializeCurveTrack(Curve));
            }
            ModelJson["Curves"] = CurvesArray;
        }
        else
        {
            ModelJson["FrameRateNumerator"] = 30;
            ModelJson["FrameRateDenominator"] = 1;
            ModelJson["NumberOfFrames"] = 0;
            ModelJson["NumberOfKeys"] = 0;
            ModelJson["PlayLength"] = 0.0f;
            ModelJson["Tracks"] = JSON::Make(JSON::Class::Array);
            ModelJson["Curves"] = JSON::Make(JSON::Class::Array);
        }

        InOutHandle["DataModel"] = ModelJson;
    }
}



