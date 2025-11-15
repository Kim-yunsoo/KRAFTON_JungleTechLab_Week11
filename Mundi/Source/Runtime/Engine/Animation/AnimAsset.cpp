#include "pch.h"
#include "AnimAsset.h"
#include "JsonSerializer.h"
#include "PathUtils.h"

namespace
{
    FString ExtractAssetNameFromPath(const FString& InPath)
    {
        if (InPath.empty()) { return FString(); }

        try
        {
            const FWideString WidePath = UTF8ToWide(InPath);
            std::filesystem::path FsPath(WidePath);
            return WideToUTF8(FsPath.stem().wstring());
        }
        catch (const std::exception&)
        {
            return FString();
        }
    }
}

void UAnimAsset::SetSourceFile(const FString& InSourceFile)
{
    if (InSourceFile.empty())
    {
        SourceFile.clear();
        return;
    }

    SourceFile = NormalizePath(InSourceFile);

    if (AssetName.empty())
    {
        AssetName = ExtractAssetNameFromPath(SourceFile);
    }
}

void UAnimAsset::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FString LoadedSource;
        if (FJsonSerializer::ReadString(InOutHandle, "SourceFile", LoadedSource, "", false))
        {
            SetSourceFile(LoadedSource);
        }
        else
        {
            SourceFile.clear();
        }

        FString LoadedAssetName;
        if (FJsonSerializer::ReadString(InOutHandle, "AssetName", LoadedAssetName, "", false))
        {
            AssetName = LoadedAssetName;
        }

        FString LoadedSkeletonName;
        if (FJsonSerializer::ReadString(InOutHandle, "SkeletonName", LoadedSkeletonName, "", false))
        {
            SkeletonName = LoadedSkeletonName;
        }

        if (AssetName.empty())
        {
            AssetName = ExtractAssetNameFromPath(SourceFile);
        }
    }
    else
    {
        InOutHandle["SourceFile"] = SourceFile.c_str();
        InOutHandle["AssetName"] = AssetName.c_str();
        InOutHandle["SkeletonName"] = SkeletonName.c_str();
    }
}