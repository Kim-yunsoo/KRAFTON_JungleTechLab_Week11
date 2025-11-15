#pragma once
#include "Object.h"
#include "fbxsdk.h"

class UAnimSequence;
class UAnimDataModel;
struct FSkeleton;
struct FSkeletalMeshData;

class UFbxLoader : public UObject
{
public:

	DECLARE_CLASS(UFbxLoader, UObject)
	static UFbxLoader& GetInstance();
	UFbxLoader();

	static void PreLoad();

	USkeletalMesh* LoadFbxMesh(const FString& FilePath);

	FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath);

	UAnimSequence* LoadAnimationFromFbx(const FString& FilePath, const FSkeleton& TargetSkeleton);

	bool BuildAnimDataModelFromFbx(const FString& FilePath, const FSkeleton& TargetSkeleton, UAnimDataModel& OutModel);
	const TArray<FString>& GetDiscoveredAnimationSources() const { return AnimationSourceFiles; }

protected:
	~UFbxLoader() override;

private:
	UFbxLoader(const UFbxLoader&) = delete;
	UFbxLoader& operator=(const UFbxLoader&) = delete;


	void LoadMeshFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TMap<FbxSurfaceMaterial*, int32>& MaterialToIndex);

	void LoadSkeletonFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, int32 ParentNodeIndex, TMap<FbxNode*, int32>& BoneToIndex);

	void LoadMesh(FbxMesh* InMesh, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TArray<int32> MaterialSlotToIndex, int32 DefaultMaterialIndex = 0);

	void ParseMaterial(FbxSurfaceMaterial* Material, FMaterialInfo& MaterialInfo);

	FString ParseTexturePath(FbxProperty& Property);

	void EnsureSingleRootBone(FSkeletalMeshData& MeshData);
	bool ExtractAnimationFromScene(FbxScene& Scene, const FSkeleton& TargetSkeleton, UAnimDataModel& OutModel);
	bool InspectFbxContent(const FString& FilePath, bool& OutHasMesh, bool& OutHasAnimation) const;
	FString MakeAnimationCacheKey(const FString& FilePath, const FString& SkeletonName) const;

	// bin파일 저장용
	TArray<FMaterialInfo> MaterialInfos;
	FbxManager* SdkManager = nullptr;
	TArray<FString> AnimationSourceFiles;
	
};
