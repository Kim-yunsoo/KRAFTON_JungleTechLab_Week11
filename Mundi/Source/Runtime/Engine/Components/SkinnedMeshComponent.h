#pragma once
#include "MeshComponent.h"
#include "SkeletalMesh.h"
#include "USkinnedMeshComponent.generated.h"

UCLASS(DisplayName="스킨드 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkinnedMeshComponent : public UMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()

    USkinnedMeshComponent();
    ~USkinnedMeshComponent() override;

    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    void DuplicateSubObjects() override;
    
// Mesh Component Section
public:
    void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;
    
    FAABB GetWorldAABB() const override;
    void OnTransformUpdated() override;

// Skeletal Section
public:
    /**
     * @brief 렌더링할 스켈레탈 메시 에셋 설정 (UStaticMeshComponent::SetStaticMesh와 동일한 역할)
     * @param PathFileName 새 스켈레탈 메시 에셋 경로
     */
    virtual void SetSkeletalMesh(const FString& PathFileName);
    /**
     * @brief 이 컴포넌트의 USkeletalMesh 에셋을 반환
     */
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

    /**
     * @brief GPU/CPU 스키닝 모드 전환
     * @param bUseGPU true면 GPU 스키닝, false면 CPU 스키닝
     */
    void SetSkinningMode(bool bUseGPU);

    /**
     * @brief 현재 스키닝 모드 반환
     */
    bool IsUsingGPUSkinning() const { return bUseGPUSkinning; }

protected:
    void PerformSkinning();
    /**
     * @brief 자식에게서 원본 메시를 받아 CPU 스키닝을 수행
     * @param InSkinningMatrices 스키닝 매트릭스
     */
    void UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices);

    /**
     * @brief GPU 스키닝용 본 매트릭스 상수 버퍼 업데이트
     */
    void UpdateBoneMatrixBuffer();

    /**
     * @brief GPU 스키닝용 리소스 생성 (Vertex Buffer + Constant Buffer)
     */
    void CreateGPUSkinningResources();

    /**
     * @brief GPU 스키닝용 리소스 해제
     */
    void ReleaseGPUSkinningResources();
    
    UPROPERTY(EditAnywhere, Category = "Skeletal Mesh", Tooltip = "Skeletal mesh asset to render")
    USkeletalMesh* SkeletalMesh;

    /**
     * @brief CPU 스키닝 최종 결과물. 렌더러가 이 데이터를 사용합니다.
     */
    TArray<FNormalVertex> SkinnedVertices;
    /**
     * @brief CPU 스키닝 최종 결과물. 렌더러가 이 데이터를 사용합니다.
     */
    TArray<FNormalVertex> NormalSkinnedVertices;

private:
    FVector SkinVertexPosition(const FSkinnedVertex& InVertex) const;
    FVector SkinVertexNormal(const FSkinnedVertex& InVertex) const;
    FVector4 SkinVertexTangent(const FSkinnedVertex& InVertex) const;

    /**
     * @brief 자식이 계산해 준, 현재 프레임의 최종 스키닝 행렬
    */
    TArray<FMatrix> FinalSkinningMatrices;
    TArray<FMatrix> FinalSkinningNormalMatrices;
    bool bSkinningMatricesDirty = true;

    /**
     * @brief CPU 스키닝에서 진행하기 때문에, Component별로 VertexBuffer를 가지고 스키닝 업데이트를 진행해야함
    */
    ID3D11Buffer* VertexBuffer = nullptr;

    // ===== GPU 스키닝 관련 멤버 =====
    /**
     * @brief GPU 스키닝 사용 여부
     */
    bool bUseGPUSkinning = false;

    /**
     * @brief GPU 스키닝용 정점 버퍼 (본 인덱스/가중치 포함, FSkinnedVertex 형식)
     */
    ID3D11Buffer* GPUSkinnedVertexBuffer = nullptr;

    /**
     * @brief GPU 스키닝용 본 매트릭스 상수 버퍼 (register b6)
     */
    ID3D11Buffer* BoneMatrixConstantBuffer = nullptr;

    /**
     * @brief GPU 리소스 소유권 플래그 (PIE 복제본은 false, 원본은 true)
     * PIE 복제본은 원본의 GPU 리소스를 공유하므로 소멸 시 해제하지 않음
     */
    bool bOwnsGPUResources = true;
};
