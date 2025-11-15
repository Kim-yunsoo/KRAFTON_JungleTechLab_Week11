#pragma once
#include "Object.h"
#include "fbxsdk.h"

enum class EAssetType
{
	SkeletalMesh,   // 메시 + 스켈레톤 (+ 선택적 애니메이션)
	AnimationOnly   // 애니메이션만 (스켈레톤 노드만 있고 메시 없음)
};

class UFbxLoader : public UObject
{
public:

	DECLARE_CLASS(UFbxLoader, UObject)
	static UFbxLoader& GetInstance();
	UFbxLoader();

	static void PreLoad();

	USkeletalMesh* LoadFbxMesh(const FString& FilePath);

	FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath);

	// 애니메이션 로딩 함수들
	TArray<FAnimationData*> LoadAnimationsFromFbx(const FString& FilePath);

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

	EAssetType DetermineAssetType(FbxScene* Scene);
	TArray<FAnimationData*> LoadAnimationsFromScene(FbxScene* Scene, const FSkeleton& Skeleton);
	void ExtractAnimCurveKeys(FbxAnimCurve* Curve, TArray<float>& OutKeys, TArray<float>& OutTimes);

	// bin파일 저장용
	TArray<FMaterialInfo> MaterialInfos;
	FbxManager* SdkManager = nullptr;
	
};