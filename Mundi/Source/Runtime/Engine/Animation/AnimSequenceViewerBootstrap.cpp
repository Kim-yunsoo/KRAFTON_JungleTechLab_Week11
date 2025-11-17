#include "pch.h"
#include "AnimSequenceViewerBootstrap.h"
#include "CameraActor.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"

ViewerState* AnimSequenceViewerBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ViewerState* State = new ViewerState();
    State->Name = Name ? Name : "AnimSequenceViewer";

    // Preview world 생성
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);  // 메모리 최적화를 위한 프리뷰 월드
    State->World->Initialize();

    // 애니메이션 프리뷰용 쇼플래그 설정 (에디터 아이콘 비활성화)
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    // Viewport 생성
    State->Viewport = new FViewport();
    // 초기 크기는 매 프레임마다 조정됨
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);

    // 기본 ViewportClient 사용 (에디터 기능 불필요)
    auto* Client = new FViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);

    // 애니메이션 전신이 보이도록 카메라 위치 조정 (더 뒤로, 약간 위에서)
    Client->GetCamera()->SetActorLocation(FVector(5, 0, 1.5));

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);

    State->World->SetEditorCameraActor(Client->GetCamera());

    // 프리뷰용 SkeletalMeshActor 스폰
    if (State->World)
    {
        ASkeletalMeshActor* Preview = State->World->SpawnActor<ASkeletalMeshActor>();
        State->PreviewActor = Preview;
    }

    return State;
}

void AnimSequenceViewerBootstrap::DestroyViewerState(ViewerState*& State)
{
    if (!State) return;
    if (State->Viewport) { delete State->Viewport; State->Viewport = nullptr; }
    if (State->Client) { delete State->Client; State->Client = nullptr; }
    if (State->World) { ObjectFactory::DeleteObject(State->World); State->World = nullptr; }
    delete State; State = nullptr;
}
