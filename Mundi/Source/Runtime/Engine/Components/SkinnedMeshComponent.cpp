#include "pch.h"
#include "SkinnedMeshComponent.h"
#include "MeshBatchElement.h"
#include "SceneView.h"

USkinnedMeshComponent::USkinnedMeshComponent() : SkeletalMesh(nullptr)
{
   bCanEverTick = true;
}

USkinnedMeshComponent::~USkinnedMeshComponent()
{
   if (VertexBuffer)
   {
      VertexBuffer->Release();
      VertexBuffer = nullptr;
   }

   ReleaseGPUSkinningResources();
}

void USkinnedMeshComponent::BeginPlay()
{
   Super::BeginPlay();
}

void USkinnedMeshComponent::TickComponent(float DeltaTime)
{
   UMeshComponent::TickComponent(DeltaTime);
}

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
   Super::Serialize(bInIsLoading, InOutHandle);

   if (bInIsLoading)
   {
      SetSkeletalMesh(SkeletalMesh->GetPathFileName());
   }
   // @TODO - UStaticMeshComponent처럼 프로퍼티 기반 직렬화 로직 추가
}

void USkinnedMeshComponent::DuplicateSubObjects()
{
   Super::DuplicateSubObjects();
   SkeletalMesh->CreateVertexBuffer(&VertexBuffer);

   // GPU 스키닝 모드였다면 GPU 리소스도 복제
   if (bUseGPUSkinning)
   {
      CreateGPUSkinningResources();

      // GPU 리소스 생성 실패 시 CPU 모드로 폴백
      if (!BoneMatrixConstantBuffer || !GPUSkinnedVertexBuffer)
      {
         UE_LOG("[error] DuplicateSubObjects: Failed to create GPU resources. Falling back to CPU skinning.");
         bUseGPUSkinning = false;
      }
   }
}

void USkinnedMeshComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData()) { return; }

   // IndexBuffer 유효성 검사 (PIE 중지 시 이미 해제되었을 수 있음)
   if (!SkeletalMesh->GetIndexBuffer())
   {
      return;
   }

   // CPU 스키닝 모드: 버텍스 버퍼 업데이트
   if (!bUseGPUSkinning)
   {
      // CPU 모드용 VertexBuffer 유효성 검사
      if (!VertexBuffer)
      {
         UE_LOG("[error] CollectMeshBatches: VertexBuffer is null in CPU mode.");
         return;
      }

      if (bSkinningMatricesDirty)
      {
         bSkinningMatricesDirty = false;
         SkeletalMesh->UpdateVertexBuffer(SkinnedVertices, VertexBuffer);
      }
   }
   // GPU 스키닝 모드: 본 매트릭스 버퍼 업데이트
   else
   {
      // GPU 리소스가 유효하지 않으면 CPU 모드로 폴백
      if (!BoneMatrixConstantBuffer || !GPUSkinnedVertexBuffer)
      {
         UE_LOG("[error] CollectMeshBatches: GPU resources are invalid. Falling back to CPU skinning.");
         bUseGPUSkinning = false;
         bSkinningMatricesDirty = true;
         PerformSkinning();

         // CPU 모드용 VertexBuffer 유효성 재검사
         if (!VertexBuffer)
         {
            UE_LOG("[error] CollectMeshBatches: VertexBuffer is null after fallback to CPU mode.");
            return;
         }

         if (bSkinningMatricesDirty)
         {
            bSkinningMatricesDirty = false;
            SkeletalMesh->UpdateVertexBuffer(SkinnedVertices, VertexBuffer);
         }
      }
      else
      {
         UpdateBoneMatrixBuffer();
      }
   }

    const TArray<FGroupInfo>& MeshGroupInfos = SkeletalMesh->GetMeshGroupInfo();
    auto DetermineMaterialAndShader = [&](uint32 SectionIndex) -> TPair<UMaterialInterface*, UShader*>
    {
       UMaterialInterface* Material = GetMaterial(SectionIndex);
       UShader* Shader = nullptr;

       if (Material && Material->GetShader())
       {
          Shader = Material->GetShader();
       }
       else
       {
          // UE_LOG("USkinnedMeshComponent: 머티리얼이 없거나 셰이더가 없어서 기본 머티리얼 사용 section %u.", SectionIndex);
          Material = UResourceManager::GetInstance().GetDefaultMaterial();
          if (Material)
          {
             Shader = Material->GetShader();
          }
          if (!Material || !Shader)
          {
             UE_LOG("USkinnedMeshComponent: 기본 머티리얼이 없습니다.");
             return { nullptr, nullptr };
          }
       }
       return { Material, Shader };
    };

    const bool bHasSections = !MeshGroupInfos.IsEmpty();
    const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;

    for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
    {
       uint32 IndexCount = 0;
       uint32 StartIndex = 0;

       if (bHasSections)
       {
          const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
          IndexCount = Group.IndexCount;
          StartIndex = Group.StartIndex;
       }
       else
       {
          IndexCount = SkeletalMesh->GetIndexCount();
          StartIndex = 0;
       }

       if (IndexCount == 0)
       {
          continue;
       }

       auto [MaterialToUse, ShaderToUse] = DetermineMaterialAndShader(SectionIndex);
       if (!MaterialToUse || !ShaderToUse)
       {
          continue;
       }

       FMeshBatchElement BatchElement;
       TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
       if (0 < MaterialToUse->GetShaderMacros().Num())
       {
          ShaderMacros.Append(MaterialToUse->GetShaderMacros());
       }

       // GPU 스키닝 모드일 때 GPU_SKINNING 매크로 추가
       if (bUseGPUSkinning)
       {
          FShaderMacro GPUSkinningMacro;
          GPUSkinningMacro.Name = "GPU_SKINNING";
          GPUSkinningMacro.Definition = "1";
          ShaderMacros.Add(GPUSkinningMacro);
       }

       FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);

       if (ShaderVariant)
       {
          BatchElement.VertexShader = ShaderVariant->VertexShader;
          BatchElement.PixelShader = ShaderVariant->PixelShader;
          BatchElement.InputLayout = ShaderVariant->InputLayout;
       }

       BatchElement.Material = MaterialToUse;

       // CPU/GPU 모드에 따라 다른 버텍스 버퍼와 스트라이드 사용
       if (bUseGPUSkinning)
       {
          BatchElement.VertexBuffer = GPUSkinnedVertexBuffer;
          BatchElement.VertexStride = sizeof(FSkinnedVertex);
          BatchElement.BoneMatrixConstantBuffer = BoneMatrixConstantBuffer;  // GPU 스키닝용 본 매트릭스 버퍼
       }
       else
       {
          BatchElement.VertexBuffer = VertexBuffer;
          BatchElement.VertexStride = SkeletalMesh->GetVertexStride();
          BatchElement.BoneMatrixConstantBuffer = nullptr;
       }

       BatchElement.IndexBuffer = SkeletalMesh->GetIndexBuffer();

       BatchElement.IndexCount = IndexCount;
       BatchElement.StartIndex = StartIndex;
       BatchElement.BaseVertexIndex = 0;
       BatchElement.WorldMatrix = GetWorldMatrix();
       BatchElement.ObjectID = InternalIndex;
       BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

       OutMeshBatchElements.Add(BatchElement);
    }
}

FAABB USkinnedMeshComponent::GetWorldAABB() const
{
   return {};
   // const FTransform WorldTransform = GetWorldTransform();
   // const FMatrix WorldMatrix = GetWorldMatrix();
   //
   // if (!SkeletalMesh)
   // {
   //    const FVector Origin = WorldTransform.TransformPosition(FVector());
   //    return FAABB(Origin, Origin);
   // }
   //
   // const FAABB LocalBound = SkeletalMesh->GetLocalBound(); // <-- 이 함수 구현 필요
   // const FVector LocalMin = LocalBound.Min;
   // const FVector LocalMax = LocalBound.Max;
   //
   // // ... (이하 AABB 계산 로직은 UStaticMeshComponent와 동일) ...
   // const FVector LocalCorners[8] = {
   //    FVector(LocalMin.X, LocalMin.Y, LocalMin.Z),
   //    FVector(LocalMax.X, LocalMin.Y, LocalMin.Z),
   //    // ... (나머지 6개 코너) ...
   //    FVector(LocalMax.X, LocalMax.Y, LocalMax.Z)
   // };
   //
   // FVector4 WorldMin4 = FVector4(LocalCorners[0].X, LocalCorners[0].Y, LocalCorners[0].Z, 1.0f) * WorldMatrix;
   // FVector4 WorldMax4 = WorldMin4;
   //
   // for (int32 CornerIndex = 1; CornerIndex < 8; ++CornerIndex)
   // {
   //    const FVector4 WorldPos = FVector4(LocalCorners[CornerIndex].X
   //       , LocalCorners[CornerIndex].Y
   //       , LocalCorners[CornerIndex].Z
   //       , 1.0f)
   //       * WorldMatrix;
   //    WorldMin4 = WorldMin4.ComponentMin(WorldPos);
   //    WorldMax4 = WorldMax4.ComponentMax(WorldPos);
   // }
   //
   // FVector WorldMin = FVector(WorldMin4.X, WorldMin4.Y, WorldMin4.Z);
   // FVector WorldMax = FVector(WorldMax4.X, WorldMax4.Y, WorldMax4.Z);
   // return FAABB(WorldMin, WorldMax);
}

void USkinnedMeshComponent::OnTransformUpdated()
{
   Super::OnTransformUpdated();
   MarkWorldPartitionDirty();
}

void USkinnedMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
   ClearDynamicMaterials();

   SkeletalMesh = UResourceManager::GetInstance().Load<USkeletalMesh>(PathFileName);

   if (VertexBuffer)
   {
      VertexBuffer->Release();
      VertexBuffer = nullptr;
   }
    
   if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
   {
      SkeletalMesh->CreateVertexBuffer(&VertexBuffer);

      const TArray<FMatrix> IdentityMatrices(SkeletalMesh->GetBoneCount(), FMatrix::Identity());
      UpdateSkinningMatrices(IdentityMatrices, IdentityMatrices);
      PerformSkinning();
      
      const TArray<FGroupInfo>& GroupInfos = SkeletalMesh->GetMeshGroupInfo();
       MaterialSlots.resize(GroupInfos.size());
       for (int i = 0; i < GroupInfos.size(); ++i)
      {
         // FGroupInfo에 InitialMaterialName이 있다고 가정
         SetMaterialByName(i, GroupInfos[i].InitialMaterialName);
      }
      MarkWorldPartitionDirty();
   }
   else
   {
      SkeletalMesh = nullptr;
      UpdateSkinningMatrices(TArray<FMatrix>(), TArray<FMatrix>());
      PerformSkinning();
   }
}

void USkinnedMeshComponent::PerformSkinning()
{
   // GPU 스키닝 모드에서는 CPU 스키닝을 수행하지 않음
   if (bUseGPUSkinning) { return; }

   if (!SkeletalMesh || FinalSkinningMatrices.IsEmpty()) { return; }
   if (!bSkinningMatricesDirty) { return; }

   const TArray<FSkinnedVertex>& SrcVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
   const int32 NumVertices = SrcVertices.Num();
   SkinnedVertices.SetNum(NumVertices);

   for (int32 Idx = 0; Idx < NumVertices; ++Idx)
   {
      const FSkinnedVertex& SrcVert = SrcVertices[Idx];
      FNormalVertex& DstVert = SkinnedVertices[Idx];

      DstVert.pos = SkinVertexPosition(SrcVert);
      DstVert.normal = SkinVertexNormal(SrcVert);
      DstVert.Tangent = SkinVertexTangent(SrcVert);
      DstVert.tex = SrcVert.UV;
   }
}

void USkinnedMeshComponent::UpdateSkinningMatrices(const TArray<FMatrix>& InSkinningMatrices, const TArray<FMatrix>& InSkinningNormalMatrices)
{
   FinalSkinningMatrices = InSkinningMatrices;
   FinalSkinningNormalMatrices = InSkinningNormalMatrices;
   bSkinningMatricesDirty = true;
}

FVector USkinnedMeshComponent::SkinVertexPosition(const FSkinnedVertex& InVertex) const
{
   FVector BlendedPosition(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
         FVector TransformedPosition = SkinMatrix.TransformPosition(InVertex.Position);
         BlendedPosition += TransformedPosition * Weight;
      }
   }

   return BlendedPosition;
}

FVector USkinnedMeshComponent::SkinVertexNormal(const FSkinnedVertex& InVertex) const
{
   FVector BlendedNormal(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningNormalMatrices[BoneIndex];
         FVector TransformedNormal = SkinMatrix.TransformVector(InVertex.Normal);
         BlendedNormal += TransformedNormal * Weight;
      }
   }

   return BlendedNormal.GetSafeNormal();
}

FVector4 USkinnedMeshComponent::SkinVertexTangent(const FSkinnedVertex& InVertex) const
{
   const FVector OriginalTangentDir(InVertex.Tangent.X, InVertex.Tangent.Y, InVertex.Tangent.Z);
   const float OriginalSignW = InVertex.Tangent.W;

   FVector BlendedTangentDir(0.f, 0.f, 0.f);

   for (int32 Idx = 0; Idx < 4; ++Idx)
   {
      const uint32 BoneIndex = InVertex.BoneIndices[Idx];
      const float Weight = InVertex.BoneWeights[Idx];

      if (Weight > 0.f)
      {
         const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
         FVector TransformedTangentDir = SkinMatrix.TransformVector(OriginalTangentDir);
         BlendedTangentDir += TransformedTangentDir * Weight;
      }
   }

   const FVector FinalTangentDir = BlendedTangentDir.GetSafeNormal();
   return { FinalTangentDir.X, FinalTangentDir.Y, FinalTangentDir.Z, OriginalSignW };
}

// ===== GPU 스키닝 함수 구현 =====

void USkinnedMeshComponent::SetSkinningMode(bool bUseGPU)
{
   if (bUseGPUSkinning == bUseGPU)
   {
      return;  // 이미 같은 모드면 아무것도 하지 않음
   }

   bUseGPUSkinning = bUseGPU;

   if (bUseGPUSkinning)
   {
      // GPU 모드로 전환: GPU 리소스 생성
      CreateGPUSkinningResources();
      UE_LOG("Switched to GPU Skinning mode");
   }
   else
   {
      // CPU 모드로 전환: GPU 리소스 해제, CPU 스키닝 재수행
      ReleaseGPUSkinningResources();
      bSkinningMatricesDirty = true;
      PerformSkinning();
      UE_LOG("Switched to CPU Skinning mode");
   }
}

void USkinnedMeshComponent::CreateGPUSkinningResources()
{
   if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
   {
      UE_LOG("[error] CreateGPUSkinningResources: SkeletalMesh is null");
      return;
   }

   ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();
   if (!Device)
   {
      UE_LOG("[error] CreateGPUSkinningResources: Device is null");
      return;
   }

   // 기존 리소스 해제
   ReleaseGPUSkinningResources();

   // 1. GPU용 Vertex Buffer 생성 (FSkinnedVertex 형식, 본 인덱스/가중치 포함)
   const TArray<FSkinnedVertex>& SkinnedVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
   const uint32 NumVertices = SkinnedVertices.Num();
   const uint32 VertexBufferSize = NumVertices * sizeof(FSkinnedVertex);

   D3D11_BUFFER_DESC VertexBufferDesc = {};
   VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;  // GPU 스키닝은 정점 데이터를 변경하지 않음
   VertexBufferDesc.ByteWidth = VertexBufferSize;
   VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
   VertexBufferDesc.CPUAccessFlags = 0;

   D3D11_SUBRESOURCE_DATA VertexInitData = {};
   VertexInitData.pSysMem = SkinnedVertices.data();

   HRESULT hr = Device->CreateBuffer(&VertexBufferDesc, &VertexInitData, &GPUSkinnedVertexBuffer);
   if (FAILED(hr))
   {
      UE_LOG("[error] CreateGPUSkinningResources: Failed to create GPU vertex buffer");
      return;
   }

   // 2. Bone Matrix Constant Buffer 생성 (register b6)
   const uint32 MaxBones = 256;
   const uint32 BoneBufferSize = MaxBones * sizeof(FMatrix);  // 128 bones * 64 bytes = 8192 bytes

   D3D11_BUFFER_DESC ConstantBufferDesc = {};
   ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
   ConstantBufferDesc.ByteWidth = (BoneBufferSize + 15) & ~15;  // 16바이트 정렬
   ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
   ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

   hr = Device->CreateBuffer(&ConstantBufferDesc, nullptr, &BoneMatrixConstantBuffer);
   if (FAILED(hr))
   {
      UE_LOG("[error] CreateGPUSkinningResources: Failed to create bone matrix constant buffer");
      if (GPUSkinnedVertexBuffer)
      {
         GPUSkinnedVertexBuffer->Release();
         GPUSkinnedVertexBuffer = nullptr;
      }
      return;
   }

   UE_LOG("GPU Skinning resources created successfully");
}

void USkinnedMeshComponent::ReleaseGPUSkinningResources()
{
   if (GPUSkinnedVertexBuffer)
   {
      GPUSkinnedVertexBuffer->Release();
      GPUSkinnedVertexBuffer = nullptr;
   }

   if (BoneMatrixConstantBuffer)
   {
      BoneMatrixConstantBuffer->Release();
      BoneMatrixConstantBuffer = nullptr;
   }
}

void USkinnedMeshComponent::UpdateBoneMatrixBuffer()
{
   // GPU 스키닝이 비활성화되었으면 리턴
   if (!bUseGPUSkinning)
   {
      return;
   }

   // 버퍼나 매트릭스 데이터가 없으면 리턴
   if (!BoneMatrixConstantBuffer || FinalSkinningMatrices.IsEmpty())
   {
      return;
   }

   ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();
   if (!Context)
   {
      return;
   }

   // SEH(Structured Exception Handling)로 예외 처리
   __try
   {
      // Constant Buffer에 본 매트릭스 업로드
      D3D11_MAPPED_SUBRESOURCE MappedResource;
      HRESULT hr = Context->Map(BoneMatrixConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

      if (FAILED(hr))
      {
         // Map 실패 시 GPU 리소스가 유효하지 않으므로 GPU 모드 비활성화
         UE_LOG("[error] UpdateBoneMatrixBuffer: Failed to map bone matrix buffer (HRESULT: 0x%X). Disabling GPU skinning.", hr);
         bUseGPUSkinning = false;
         ReleaseGPUSkinningResources();
         return;
      }

      const uint32 NumBones = FinalSkinningMatrices.Num();
      const uint32 MaxBones = 256;
      const uint32 BonesToCopy = (NumBones < MaxBones) ? NumBones : MaxBones;

      // 본 매트릭스 복사
      memcpy(MappedResource.pData, FinalSkinningMatrices.data(), BonesToCopy * sizeof(FMatrix));

      Context->Unmap(BoneMatrixConstantBuffer, 0);
   }
   __except (EXCEPTION_EXECUTE_HANDLER)
   {
      // 예외 발생 시 GPU 모드 비활성화 및 리소스 해제
      UE_LOG("[error] UpdateBoneMatrixBuffer: Exception occurred during buffer mapping. Disabling GPU skinning.");
      bUseGPUSkinning = false;
      ReleaseGPUSkinningResources();
   }
}
