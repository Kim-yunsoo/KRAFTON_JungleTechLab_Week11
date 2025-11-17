#include "pch.h"
#include "USlateManager.h"
#include "AnimSequenceViewerWindow.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"
#include "Source/Runtime/Engine/Animation/AnimSequenceViewerBootstrap.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "FViewport.h"
#include "FViewportClient.h"
SAnimSequenceViewerWindow::SAnimSequenceViewerWindow()
{
    // ResourceManager에서 모든 애니메이션 파일 경로 가져오기
    UResourceManager& ResourceManager = UResourceManager::GetInstance();
    AvailableAnimationPaths = ResourceManager.GetAllFilePaths<UAnimSequence>();
    
    CurrentSequence = nullptr; // 초기 시퀀스 null (아무것도 선택 안됨)

	//// 더미 시퀀스 생성 (테스트용)
	//CurrentSequence = NewObject<UAnimSequenceBase>();
	//CurrentSequence->SetSequenceLength(5.0f);
	//CurrentSequence->SetLooping(true);

	//// 더미 Notify 데이터 추가
	//FAnimNotifyEvent notify1;
	//notify1.TriggerTime = 1.0f;
	//notify1.Duration = 0.3f;
	//notify1.NotifyName = "Footstep_L";
	//CurrentSequence->AddNotify(notify1);

	//FAnimNotifyEvent notify2;
	//notify2.TriggerTime = 2.0f;
	//notify2.Duration = 0.0f;
	//notify2.NotifyName = "Footstep_R";
	//CurrentSequence->AddNotify(notify2);

	//FAnimNotifyEvent notify3;
	//notify3.TriggerTime = 3.5f;
	//notify3.Duration = 0.5f;  // Duration이 있는 경우
	//notify3.NotifyName = "PlaySound";
	//CurrentSequence->AddNotify(notify3);

	//FAnimNotifyEvent notify4;
	//notify4.TriggerTime = 4.2f;
	//notify4.Duration = 0.0f;
	//notify4.NotifyName = "SpawnEffect";
	//CurrentSequence->AddNotify(notify4);
}

SAnimSequenceViewerWindow::~SAnimSequenceViewerWindow()
{
	// ViewerState 파괴
	AnimSequenceViewerBootstrap::DestroyViewerState(PreviewState);

	// 시퀀스 정리
	if (CurrentSequence)
	{
		CurrentSequence = nullptr;
	}
}

bool SAnimSequenceViewerWindow::Initialize(UWorld* InWorld, ID3D11Device* InDevice)
{
	World = InWorld;
	Device = InDevice;

	// ViewerState 생성 (AnimSequenceViewer 전용 부트스트랩 사용)
	PreviewState = AnimSequenceViewerBootstrap::CreateViewerState("AnimSequencePreview", InWorld, InDevice);
	if (!PreviewState)
	{
		return false;
	}

	return true;
}

void SAnimSequenceViewerWindow::SetSkeletalMeshPath(const char* MeshPath)
{
	if (!PreviewState || !MeshPath || MeshPath[0] == '\0')
	{
		return;
	}

	// MeshPathBuffer에 경로 복사
	strncpy_s(PreviewState->MeshPathBuffer, MeshPath, sizeof(PreviewState->MeshPathBuffer) - 1);
	PreviewState->MeshPathBuffer[sizeof(PreviewState->MeshPathBuffer) - 1] = '\0';

	// 즉시 스켈레탈 메시 로드
	if (PreviewState->PreviewActor)
	{
		ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
		if (PreviewActor)
		{
			PreviewActor->SetSkeletalMesh(MeshPath);
			UE_LOG("[AnimSequenceViewer] Skeletal mesh set from outliner: %s", MeshPath);

			// 현재 애니메이션이 있고 재생 중이면 다시 재생
			if (CurrentSequence && bIsPlaying)
			{
				UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentSequence);
				if (AnimSequence)
				{
					USkeletalMeshComponent* SkeletalMeshComp = PreviewActor->GetSkeletalMeshComponent();
					if (SkeletalMeshComp)
					{
						SkeletalMeshComp->SetVisibility(true);
						SkeletalMeshComp->PlayAnimation(AnimSequence, bLooping);
					}
				}
			}
		}
	}
}

void SAnimSequenceViewerWindow::LoadAnimSquence(UAnimSequence* Sequence)
{
    if (!Sequence)
    {
        UE_LOG("[AnimSequenceViewer] LoadAnimSequence: Sequence is null");
        return;
    }

    // 기존 시퀀스 교체
    CurrentSequence = Sequence;

    // 시퀀스 정보로 타임라인 업데이트
    PlayLength = Sequence->GetPlayLength();
    TotalFrames = Sequence->GetNumberOfFrames();
    bLooping = Sequence->IsLooping();

    // 재생 상태 초기화
    CurrentTime = 0.0f;
    CurrentFrame = 0;
    bIsPlaying = false;

    UE_LOG("[AnimSequenceViewer] Loaded: %s (Length: %.2fs, Frames: %d)",
        Sequence->GetFilePath().c_str(), PlayLength, TotalFrames);

    // PreviewActor에 애니메이션 설정 (아직 재생하지 않음)
    // 참고: 스켈레탈 메시는 이미 SetSkeletalMeshPath()에서 설정되어 있음
    if (PreviewState && PreviewState->PreviewActor)
    {
        ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
        if (PreviewActor)
        {
            USkeletalMeshComponent* SkeletalMeshComp = PreviewActor->GetSkeletalMeshComponent();
            if (SkeletalMeshComp)
            {
                // 메시를 보이게만 하고 애니메이션은 재생하지 않음 (사용자가 Play 버튼 눌러야 함)
                SkeletalMeshComp->SetVisibility(true);
                UE_LOG("[AnimSequenceViewer] Animation loaded. Press Play to start.");
            }
            else
            {
                UE_LOG("[AnimSequenceViewer] WARNING: No skeletal mesh loaded. Please select a skeletal mesh from the outliner first.");
            }
        }
        else
        {
            UE_LOG("[AnimSequenceViewer] ERROR: PreviewActor cast failed");
        }
    }
    else
    {
        UE_LOG("[AnimSequenceViewer] ERROR: PreviewState or PreviewActor is null");
    }
}

void SAnimSequenceViewerWindow::OnRender()
{
	if (!bIsOpen) return;

	// 처음 한 번만 윈도우 위치/크기 설정
	if (!bInitialPlacementDone)
	{
		ImGui::SetNextWindowPos(ImVec2(100, 100));
		ImGui::SetNextWindowSize(ImVec2(1200, 800));  // 크기 조정 (프리뷰를 위해 더 크게)
		bInitialPlacementDone = true;
	}

    // 윈도우 시작 (사용자가 X버튼 누르면 bIsOpen이 false가 됨)
    if (ImGui::Begin("Animation Sequence Viewer", &bIsOpen))
    {
        // 윈도우 Rect 업데이트 (마우스 이벤트 라우팅용)
        ImVec2 WindowPos = ImGui::GetWindowPos();
        ImVec2 WindowSize = ImGui::GetWindowSize();
        Rect.Left = WindowPos.x;
        Rect.Top = WindowPos.y;
        Rect.Right = WindowPos.x + WindowSize.x;
        Rect.Bottom = WindowPos.y + WindowSize.y;
        Rect.UpdateMinMax();

        ImVec2 ContentAvail = ImGui::GetContentRegionAvail();
        float TotalWidth = ContentAvail.x;
        float TotalHeight = ContentAvail.y;

        // ============================================================
        // 레이아웃 비율 계산
        // ============================================================
        float TopPreviewHeightPixels = TotalHeight * TopPreviewHeight;      // 상단 60%
        float BottomPanelHeightPixels = TotalHeight * BottomPanelHeight;    // 하단 40%

        // 하단 패널의 가로 분할
        float LeftNotifyWidthPixels = TotalWidth * LeftNotifyWidth;         // 좌측 15%
        float CenterTimelineWidthPixels = TotalWidth * CenterTimelineWidth; // 중앙 55%
        float RightPanelWidthPixels = TotalWidth * RightPanelWidth;         // 우측 30%

        // 우측 패널의 세로 분할
        float RightTopInfoHeightPixels = BottomPanelHeightPixels * RightTopInfoHeight;      // 40%
        float RightBottomListHeightPixels = BottomPanelHeightPixels * RightBottomListHeight; // 60%

        // ============================================================
        // 상단: 3D 프리뷰 뷰포트 (전체 너비, 60% 높이)
        // ============================================================
        RenderPreviewViewport(TopPreviewHeightPixels);

        // ============================================================
        // 하단: 좌측 (통합 Notify+Timeline) | 우측 (Info+List)
        // ============================================================

        // 스타일: 패널 간 간격 제거
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // --- 좌측: 통합 Notify+Timeline 패널 (70%) ---
        float LeftCombinedWidth = TotalWidth * 0.70f; // Notify+Timeline 합쳐서 70%
        ImGui::BeginChild("CombinedNotifyTimelinePanel", ImVec2(LeftCombinedWidth, BottomPanelHeightPixels), true);
        {
            // Notify 트랙과 Timeline을 합친 새로운 레이아웃
            RenderCombinedNotifyTimeline();
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // --- 우측: Info + List (30%, 세로 분할) ---
        ImGui::BeginChild("RightPanel", ImVec2(RightPanelWidthPixels, BottomPanelHeightPixels), false, ImGuiWindowFlags_NoScrollbar);
        {
            // 상단: Animation Info (40%)
            ImGui::BeginChild("InfoPanel", ImVec2(0, RightTopInfoHeightPixels), true);
            {
                RenderInfoPanel();
            }
            ImGui::EndChild();

            // 하단: Animation List (60%)
            ImGui::BeginChild("AnimList", ImVec2(0, RightBottomListHeightPixels), true);
            {
                RenderAnimationList();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(); // ItemSpacing 복원
    }
    ImGui::End();

    // 윈도우가 닫히면 정리
    if (!bIsOpen)
    {
        // USlateManager::OnRender()에서 처리됨
    }
}

void SAnimSequenceViewerWindow::OnUpdate(float DeltaSeconds)
{
    // ViewerState 업데이트 (월드 틱)
    if (PreviewState && PreviewState->World)
    {
        PreviewState->World->Tick(DeltaSeconds);
    }

    // ViewportClient 업데이트 (카메라 컨트롤)
    if (PreviewState && PreviewState->Client)
    {
        PreviewState->Client->Tick(DeltaSeconds);
    }

    // 타임라인 UI 업데이트
    if (bIsPlaying && CurrentSequence)
    {
        // 시간 증가/감소 (PlayRate에 따라 정재생/역재생)
        CurrentTime += DeltaSeconds * PlayRate;

        // 정재생 (PlayRate > 0): 앞으로 재생
        if (PlayRate > 0.0f)
        {
            if (CurrentTime > PlayLength)
            {
                if (bLooping)
                {
                    // 루프: 처음으로 되돌림
                    // ex) CurrentTime = 5.3, PlayLength = 5.0 -> 0.3 (나머지 반환)
                    CurrentTime = fmod(CurrentTime, PlayLength);
                }
                else
                {
                    // 루프 아님: 끝에서 정지
                    CurrentTime = PlayLength;
                    bIsPlaying = false;
                }
            }
        }
        // 역재생 (PlayRate < 0): 뒤로 재생
        else if (PlayRate < 0.0f)
        {
            if (CurrentTime < 0.0f)
            {
                if (bLooping)
                {
                    // 루프: 끝으로 이동
                    CurrentTime = PlayLength + fmod(CurrentTime, PlayLength);
                }
                else
                {
                    // 루프 아님: 처음에서 정지
                    CurrentTime = 0.0f;
                    bIsPlaying = false;
                }
            }
        }

        // 프레임 업데이트
        CurrentFrame = TimeToFrame(CurrentTime);

        // 애니메이션 포즈를 직접 평가하여 SkeletalMeshComponent에 적용 (재생 중일 때만)
        ApplyAnimationPose();
    }
}

void SAnimSequenceViewerWindow::ApplyAnimationPose()
{
    if (!CurrentSequence || !PreviewState || !PreviewState->PreviewActor)
    {
        return;
    }

    ASkeletalMeshActor* PreviewActor = Cast<ASkeletalMeshActor>(PreviewState->PreviewActor);
    if (!PreviewActor)
    {
        return;
    }

    USkeletalMeshComponent* SkelComp = PreviewActor->GetSkeletalMeshComponent();
    if (!SkelComp)
    {
        return;
    }

    USkeletalMesh* SkelMesh = SkelComp->GetSkeletalMesh();
    if (!SkelMesh || !SkelMesh->GetSkeletalMeshData())
    {
        return;
    }

    const FSkeleton& Skeleton = SkelMesh->GetSkeletalMeshData()->Skeleton;
    int32 BoneCount = Skeleton.Bones.Num();

    // 애니메이션 시퀀스로부터 포즈 평가
    UAnimSequence* AnimSeq = Cast<UAnimSequence>(CurrentSequence);
    if (AnimSeq)
    {
        TArray<FTransform> BonePoses;
        BonePoses.resize(BoneCount);

        // 레퍼런스 포즈로 초기화
        for (int32 i = 0; i < BoneCount; ++i)
        {
            BonePoses[i] = FTransform(Skeleton.Bones[i].BindPose);
        }

        // 애니메이션 트랙 데이터로 오버라이드
        float FrameRate = AnimSeq->GetFrameRate();
        for (const FBoneAnimationTrack& Track : AnimSeq->GetBoneTracks())
        {
            if (Track.BoneIndex >= 0 && Track.BoneIndex < BoneCount)
            {
                BonePoses[Track.BoneIndex] = Track.InternalTrack.GetTransform(FrameRate, CurrentTime);
            }
        }

        // SkeletalMeshComponent에 포즈 직접 설정
        SkelComp->SetLocalSpacePose(BonePoses);
    }
}

void SAnimSequenceViewerWindow::RenderAnimationList()
{
    ImGui::Text("Animation List");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Available Animations:");
    ImGui::Spacing();

    // 실제 애니메이션 파일 목록 표시
    for (int i = 0; i < AvailableAnimationPaths.size(); i++)
    {
        bool bIsSelected = (SelectedAnimIndex == i);

        // 파일 경로에서 파일명만 추출 (확장자 제거)
        FString FullPath = AvailableAnimationPaths[i];

        // 1. 경로에서 파일명 추출
        size_t LastSlash = FullPath.find_last_of("/\\");
        FString FileName = (LastSlash != FString::npos)
            ? FullPath.substr(LastSlash + 1)
            : FullPath;

        // 2. 확장자 제거
        size_t LastDot = FileName.find_last_of(".");
        if (LastDot != FString::npos)
        {
            FileName = FileName.substr(0, LastDot);
        }
        if (ImGui::Selectable(FileName.c_str(), bIsSelected))
        {
            SelectedAnimIndex = i;

            // 실제 애니메이션 시퀀스 로드 (선택된 것)
            UResourceManager& ResourceManager = UResourceManager::GetInstance();
            UAnimSequence* LoadedAnim = ResourceManager.Load<UAnimSequence>(AvailableAnimationPaths[i]);

            if (LoadedAnim)
            {
                // 선택된 애니메이션 시퀀스 로드하기
                LoadAnimSquence(LoadedAnim);
            }
        }
        if (bIsSelected)
        {
            ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 선택 정보
    if (SelectedAnimIndex >= 0 && SelectedAnimIndex < AvailableAnimationPaths.size())
    {
        // 파일명 추출 (확장자 제거)
        FString FullPath = AvailableAnimationPaths[SelectedAnimIndex];
        size_t LastSlash = FullPath.find_last_of("/\\");
        FString FileName = (LastSlash != FString::npos)
            ? FullPath.substr(LastSlash + 1)
            : FullPath;

        size_t LastDot = FileName.find_last_of(".");
        if (LastDot != FString::npos)
            FileName = FileName.substr(0, LastDot);

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
            "Selected: %s", FileName.c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "No animation selected");
    }
    //// 임시 하드코딩된 애니메이션 목록
    //const char* DummyAnims[] = {
    //    "MM_Idle",
    //    "MM_Walk",
    //    "MM_Run",
    //    "MM_Jump"
    //};
    //
    //for (int i = 0; i < 4; i++)
    //{
    //    bool bIsSelected = (SelectedAnimIndex == i);
    //
    //    if (ImGui::Selectable(DummyAnims[i], bIsSelected))
    //    {
    //        SelectedAnimIndex = i;
    //        // Step 4+에서 실제 애니메이션 로드
    //    }
    //
    //    if (bIsSelected)
    //    {
    //        ImGui::SetItemDefaultFocus();
    //    }
    //}
    //
    //ImGui::Spacing();
    //ImGui::Separator();
    //ImGui::Spacing();
    //
    //// 선택 정보
    //if (SelectedAnimIndex >= 0)
    //{
    //    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
    //        "Selected: %s", DummyAnims[SelectedAnimIndex]);
    //}
    //else
    //{
    //    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
    //        "No animation selected");
    //}
}

void SAnimSequenceViewerWindow::RenderInfoPanel()
{
    ImGui::Text("Animation Information");
    ImGui::Separator();
    ImGui::Spacing();

    // Step 2: 플레이스홀더 정보
    if (SelectedAnimIndex >= 0)
    {
        // 실제 시퀀스 정보 표시
        UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentSequence);

        // 파일명 추출
        FString FilePath = CurrentSequence->GetFilePath();
        size_t LastSlash = FilePath.find_last_of("/\\");
        FString FileName = (LastSlash != FString::npos)
            ? FilePath.substr(LastSlash + 1)
            : FilePath;

        size_t LastDot = FileName.find_last_of(".");
        if (LastDot != FString::npos)
            FileName = FileName.substr(0, LastDot);

        ImGui::Text("Name: %s", FileName.c_str());
        ImGui::Text("Length: %.2f seconds", CurrentSequence->GetPlayLength());
        if (AnimSequence)
        {
            ImGui::Text("Frames: %d frames", AnimSequence->GetNumberOfFrames());
            ImGui::Text("FPS: %.2f", AnimSequence->GetFrameRate());
        }

        ImGui::Text("Looping: %s", CurrentSequence->IsLooping() ? "Yes" : "No");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 상세 정보
        ImGui::Text("Details:");
        ImGui::BulletText("File Path: %s", FilePath.c_str());

        if (AnimSequence)
        {
            ImGui::BulletText("Bone Tracks: %d", AnimSequence->GetBoneTracks().size());
        }

        ImGui::BulletText("Notify Events: %d", CurrentSequence->GetNotifies().size());

        //// 임시 정보 표시
        //ImGui::Text("Name: MM_Animation_%d", SelectedAnimIndex);
        //ImGui::Text("Length: 2.50 seconds (placeholder)");
        //ImGui::Text("Frames: 75 frames (placeholder)");
        //ImGui::Text("FPS: 30 (placeholder)");

        //ImGui::Spacing();
        //ImGui::Separator();
        //ImGui::Spacing();

        //// 상세 정보
        //ImGui::Text("Details:");
        //ImGui::BulletText("File Path: (not loaded)");
        //ImGui::BulletText("Bone Tracks: 0");
        //ImGui::BulletText("Notify Events: 0");
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select an animation from the list to view details");
        ImGui::PopStyleColor();
    }
}

void SAnimSequenceViewerWindow::RenderPlaybackControls()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 중앙 정렬을 위한 계산
    float WindowWidth = ImGui::GetContentRegionAvail().x;
    float ButtonWidth = 40.0f;
    float Spacing = 8.0f;
    float TotalWidth = (ButtonWidth * 5) + (Spacing * 4); // 5개 버튼 + 4개 간격
    float StartX = (WindowWidth - TotalWidth) * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + StartX);

    // 재생 컨트롤 버튼들
    ImGui::BeginGroup();
    {
        // Previous Frame 버튼
        if (ImGui::Button("|<<", ImVec2(ButtonWidth, 30)))
        {
            // TODO: 이전 프레임으로 이동
            if (CurrentFrame > 0)
            {
                CurrentFrame--;
                CurrentTime = FrameToTime(CurrentFrame);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Previous Frame");

        ImGui::SameLine(0, Spacing);

        // Play/Pause 버튼
        const char* playButtonText = bIsPlaying ? "||" : ">";
        if (ImGui::Button(playButtonText, ImVec2(ButtonWidth, 30)))
        {
            if (CurrentSequence)
            {
                bIsPlaying = !bIsPlaying;
                UE_LOG("[AnimSequenceViewer] %s", bIsPlaying ? "Playing" : "Paused");
            }
            else
            {
                UE_LOG("[AnimSequenceViewer] Cannot play: No animation selected");
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(bIsPlaying ? "Pause" : "Play");

        ImGui::SameLine(0, Spacing);

        // Stop 버튼
        if (ImGui::Button("[]", ImVec2(ButtonWidth, 30)))
        {
            // 정지 (처음으로)
            bIsPlaying = false;
            CurrentFrame = 0;
            CurrentTime = 0.0f;
            UE_LOG("[AnimSequenceViewer] Stopped and reset to frame 0");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stop");

        ImGui::SameLine(0, Spacing);

        // Next Frame 버튼
        if (ImGui::Button(">>|", ImVec2(ButtonWidth, 30)))
        {
            // TODO: 다음 프레임으로 이동
            if (CurrentFrame < TotalFrames - 1)
            {
                CurrentFrame++;
                CurrentTime = FrameToTime(CurrentFrame);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Next Frame");

        ImGui::SameLine(0, Spacing);

        // Loop Toggle 버튼
        ImVec4 loopColor = bLooping ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, loopColor);
        if (ImGui::Button("Loop", ImVec2(ButtonWidth, 30)))
        {
            bLooping = !bLooping;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(bLooping ? "Loop: ON" : "Loop: OFF");
    }
    ImGui::EndGroup();

    ImGui::Spacing();

    // 프레임 정보 표시
    ImGui::Text("Frame: %d / %d  |  Time: %.2fs / %.2fs  |  Speed: %.2fx",
        CurrentFrame, TotalFrames, CurrentTime, PlayLength, PlayRate);

    ImGui::Spacing();

    // 재생 속도 슬라이더
    ImGui::SetNextItemWidth(200.0f);
    ImGui::SliderFloat("Playback Speed", &PlayRate, -1.0f, 2.0f, "%.2fx");
}

void SAnimSequenceViewerWindow::RenderTimeline()
{
    ImGui::Text("Timeline");
    ImGui::Spacing();

    // 타임라인 영역 크기 계산
    TimelineWidth = ImGui::GetContentRegionAvail().x - 20.0f;
    float TrackHeight = 25.0f; // 각 트랙의 높이
    float RulerHeight = 35.0f; // 하단 프레임 눈금 영역 높이
    float TotalHeight = (NotifyTrackIndices.Num() * TrackHeight) + RulerHeight;

    ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    ImVec2 CanvasSize(TimelineWidth, TotalHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // 타임라인 배경
    ImVec4 bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    DrawList->AddRectFilled(CanvasPos,
        ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
        ImGui::ColorConvertFloat4ToU32(bgColor));

    // 적응형 눈금 간격 계산 (화면에 8-12개 눈금 표시)
    int targetRulerCount = 10;
    int FrameInterval = (TotalFrames > 0) ? (TotalFrames / targetRulerCount) : 10;

    // 5, 10, 20, 30, 50, 100 등 깔끔한 숫자로 반올림
    if (FrameInterval <= 5)
        FrameInterval = 5;
    else if (FrameInterval <= 10)
        FrameInterval = 10;
    else if (FrameInterval <= 20)
        FrameInterval = 20;
    else if (FrameInterval <= 30)
        FrameInterval = 30;
    else if (FrameInterval <= 50)
        FrameInterval = 50;
    else if (FrameInterval <= 100)
        FrameInterval = 100;
    else
        FrameInterval = ((FrameInterval + 99) / 100) * 100; // 100 단위로 올림

    // 프레임 눈금 그리기
    for (int frame = 0; frame <= TotalFrames; frame += FrameInterval)
    {
        float Time = FrameToTime(frame);
        float XPos = CanvasPos.x + TimeToPixel(Time);

        // 큰 눈금선
        DrawList->AddLine(
            ImVec2(XPos, CanvasPos.y + CanvasSize.y - 15),
            ImVec2(XPos, CanvasPos.y + CanvasSize.y),
            IM_COL32(150, 150, 150, 255), 2.0f);

        // 프레임 번호 표시
        char Label[16];
        sprintf_s(Label, "%d", frame);
        DrawList->AddText(
            ImVec2(XPos - 10, CanvasPos.y + CanvasSize.y - 35),
            IM_COL32(200, 200, 200, 255), Label);
    }

    // 작은 눈금 (큰 눈금 간격의 1/5 또는 1/2)
    int smallInterval = (FrameInterval >= 50) ? (FrameInterval / 5) : (FrameInterval / 2);
    if (smallInterval > 0)
    {
        for (int frame = 0; frame <= TotalFrames; frame += smallInterval)
        {
            if (frame % FrameInterval == 0) continue; // 큰 눈금은 건너뛰기

            float Time = FrameToTime(frame);
            float XPos = CanvasPos.x + TimeToPixel(Time);

            DrawList->AddLine(
                ImVec2(XPos, CanvasPos.y + CanvasSize.y - 8),
                ImVec2(XPos, CanvasPos.y + CanvasSize.y),
                IM_COL32(100, 100, 100, 255), 1.0f);
        }
    }

    // 재생 헤드 (Playhead)
    float PlayheadX = CanvasPos.x + TimeToPixel(CurrentTime);

    // 재생 헤드 라인
    DrawList->AddLine(
        ImVec2(PlayheadX, CanvasPos.y),
        ImVec2(PlayheadX, CanvasPos.y + CanvasSize.y),
        IM_COL32(255, 100, 100, 255), 3.0f);

    // 재생 헤드 상단 삼각형
    ImVec2 triangle[3] = {
        ImVec2(PlayheadX, CanvasPos.y),
        ImVec2(PlayheadX - 6, CanvasPos.y + 10),
        ImVec2(PlayheadX + 6, CanvasPos.y + 10)
    };
    DrawList->AddTriangleFilled(triangle[0], triangle[1], triangle[2],
        IM_COL32(255, 100, 100, 255));

    // 타임라인 클릭/드래그 감지
    ImGui::SetCursorScreenPos(CanvasPos);
    ImGui::InvisibleButton("TimelineButton", CanvasSize);

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
    {
        bIsDraggingPlayhead = true;
        ImVec2 MousePos = ImGui::GetMousePos();
        float ClickX = MousePos.x - CanvasPos.x;
        CurrentTime = PixelToTime(ClickX);
        CurrentFrame = TimeToFrame(CurrentTime);

        // 드래그 중에는 재생 중지
        bIsPlaying = false;
    }
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 MousePos = ImGui::GetMousePos();
        float ClickX = MousePos.x - CanvasPos.x;
        CurrentTime = PixelToTime(ClickX);
        CurrentFrame = TimeToFrame(CurrentTime);
        bIsPlaying = false;
    }
    else
    {
        bIsDraggingPlayhead = false;
    }

    // 타임라인 아래로 커서 이동
    ImGui::SetCursorScreenPos(ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y + 5));
    ImGui::Spacing();
}

void SAnimSequenceViewerWindow::RenderNotifyTrackPanel()
{
	// === 헤더: "노티파이" + 추가/삭제 버튼 ===
	ImGui::Text("Notify");
	ImGui::SameLine();

	// 드롭다운 메뉴 버튼
	if (ImGui::Button("[+] Add Track"))
	{
		ImGui::OpenPopup("AddNotifyTrackMenu");
	}

	ImGui::SameLine();

	// Delete Track 버튼 (선택된 트랙이 있을 때만 활성화)
	bool bCanDelete = (SelectedTrackIndex >= 0 && SelectedTrackIndex < NotifyTrackIndices.Num());
	if (!bCanDelete)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("[-] Delete Track"))
	{
		if (bCanDelete)
		{
			// 선택된 트랙 삭제
			NotifyTrackIndices.RemoveAt(SelectedTrackIndex);
			SelectedTrackIndex = -1; // 선택 해제
		}
	}

	if (!bCanDelete)
	{
		ImGui::EndDisabled();
	}

	// 드롭다운 메뉴
	if (ImGui::BeginPopup("AddNotifyTrackMenu"))
	{
		if (ImGui::MenuItem("Add Notify Track"))
		{
			// 새 트랙 추가
			NotifyTrackIndices.Add(NextNotifyTrackNumber);
			NextNotifyTrackNumber++;
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();
	ImGui::Spacing();

	// === Notify 트랙 목록 영역 (스크롤 가능) ===
	float PanelHeight = ImGui::GetWindowSize().y; // 전체 패널 높이
	float HeaderHeight = 60.0f; // "Notify [+] Add Track" 헤더 영역
	float PlaybackControlsHeight = 110.0f; // Playback Controls 높이
	float TrackListHeight = PanelHeight - HeaderHeight - PlaybackControlsHeight - 20.0f; // 여유 공간

	// 트랙 목록 (상단, 스크롤 가능) - Timeline과 스크롤 동기화
	ImGui::BeginChild("NotifyTrackList", ImVec2(0, TrackListHeight), false, ImGuiWindowFlags_NoScrollbar);
	{
		// 최소 높이 보장 (트랙이 없어도 전체 높이 채우기)
		float TrackHeight = 25.0f;

		// 실제 트랙 표시
		for (int i = 0; i < NotifyTrackIndices.Num(); i++)
		{
			int32 TrackNumber = NotifyTrackIndices[i];
			char Label[32];
			sprintf_s(Label, "%d", TrackNumber);

			// 트랙 번호 표시 (클릭해서 선택 가능)
			bool bSelected = (i == SelectedTrackIndex);
			if (ImGui::Selectable(Label, bSelected, 0, ImVec2(0, TrackHeight)))
			{
				// 클릭하면 선택
				SelectedTrackIndex = i;
			}

			// 호버 감지 (시각적 피드백용)
			if (ImGui::IsItemHovered())
			{
				HoveredTrackIndex = i;
			}
			else if (HoveredTrackIndex == i)
			{
				HoveredTrackIndex = -1;
			}
		}

		// 트랙이 없으면 안내 메시지
		if (NotifyTrackIndices.Num() == 0)
		{
			ImGui::TextDisabled("No notify tracks");
			ImGui::TextDisabled("Click [+] to add");
		}
	}
	ImGui::EndChild();

	// === Playback Controls를 하단에 딱 붙이기 ===
	// 현재 윈도우의 하단 위치 계산
	float WindowBottom = ImGui::GetWindowSize().y;
	float PlaybackStartY = WindowBottom - PlaybackControlsHeight+30.0f; // 하단에 붙도록

	ImGui::SetCursorPosY(PlaybackStartY);

	// 구분선
	ImGui::Separator();

	// Playback Controls
	RenderPlaybackControls();
}

float SAnimSequenceViewerWindow::TimeToPixel(float Time) const
{
    if (PlayLength <= 0.0f) return 0.0f;
    return (Time / PlayLength) * TimelineWidth;
}

float SAnimSequenceViewerWindow::PixelToTime(float PixelX) const
{
    if (TimelineWidth <= 0.0f) return 0.0f;
    float Time = (PixelX / TimelineWidth) * PlayLength;
    return FMath::Clamp(Time, 0.0f, PlayLength);
}

float SAnimSequenceViewerWindow::FrameToTime(int32 Frame) const
{
    if (TotalFrames <= 0) return 0.0f;
    return ((float)Frame / (float)TotalFrames) * PlayLength;
}

int32 SAnimSequenceViewerWindow::TimeToFrame(float Time) const
{
    if (PlayLength <= 0.0f) return 0;
    return (int32)((Time / PlayLength) * (float)TotalFrames);
}

void SAnimSequenceViewerWindow::RenderPreviewViewport(float Height)
{
    // 프리뷰 뷰포트 영역 정의 (경계선 포함)
    ImGui::BeginChild("PreviewViewport", ImVec2(0, Height), true, ImGuiWindowFlags_NoScrollbar);

    // 현재 영역의 화면 좌표와 크기 저장 (OnRenderViewport에서 사용)
    ImVec2 childPos = ImGui::GetWindowPos();
    ImVec2 childSize = ImGui::GetWindowSize();

    // PreviewRect 업데이트
    PreviewRect.Left = childPos.x;
    PreviewRect.Top = childPos.y;
    PreviewRect.Right = childPos.x + childSize.x;
    PreviewRect.Bottom = childPos.y + childSize.y;
    PreviewRect.UpdateMinMax();

    // 뷰포트가 없으면 플레이스홀더 표시
    if (!PreviewState || !PreviewState->Viewport)
    {
        ImGui::Text("Preview Viewport (No ViewerState)");
    }

    ImGui::EndChild();
}

void SAnimSequenceViewerWindow::OnMouseMove(FVector2D MousePos)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    if (PreviewRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);
        PreviewState->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SAnimSequenceViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    if (PreviewRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);
        PreviewState->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SAnimSequenceViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!PreviewState || !PreviewState->Viewport) return;

    if (PreviewRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(PreviewRect.Left, PreviewRect.Top);
        PreviewState->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SAnimSequenceViewerWindow::OnRenderViewport()
{
    // 뷰포트 렌더링 (ImGui 렌더링 전에 호출됨)
    if (PreviewState && PreviewState->Viewport && PreviewRect.GetWidth() > 0 && PreviewRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(PreviewRect.Left);
        const uint32 NewStartY = static_cast<uint32>(PreviewRect.Top);
        const uint32 NewWidth = static_cast<uint32>(PreviewRect.Right - PreviewRect.Left);
        const uint32 NewHeight = static_cast<uint32>(PreviewRect.Bottom - PreviewRect.Top);

        // 뷰포트 크기 조정
        PreviewState->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // 뷰포트 렌더링 (3D 씬)
        PreviewState->Viewport->Render();
    }
}

// ============================================================
// 통합 Notify+Timeline 패널 (언리얼 스타일)
// ============================================================

void SAnimSequenceViewerWindow::RenderCombinedNotifyTimeline()
{
    // 전체 패널 크기
    float PanelWidth = ImGui::GetContentRegionAvail().x;
    float PanelHeight = ImGui::GetContentRegionAvail().y;

    // 레이아웃 설정
    float HeaderHeight = 60.0f;         // "Notify [+] [-]" 헤더 영역
    float PlaybackHeight = 80.0f;       // Playback Controls 영역 (줄임)
    float ScrollableHeight = PanelHeight - HeaderHeight - PlaybackHeight;

    float NotifyColumnWidth = PanelWidth * 0.15f;  // Notify 트랙 번호 컬럼 15%
    float TimelineColumnWidth = PanelWidth * 0.85f; // Timeline 컬럼 85%
    float RowHeight = 25.0f; // 각 트랙 행 높이

    // ============================================================
    // 1. 헤더 영역 (고정, 스크롤 안됨)
    // ============================================================
    ImGui::Text("Notify");
    ImGui::SameLine();

    // Add Track 버튼
    if (ImGui::Button("[+] Add Track"))
    {
        ImGui::OpenPopup("AddNotifyTrackMenu");
    }

    ImGui::SameLine();

    // Delete Track 버튼
    bool bCanDelete = (SelectedTrackIndex >= 0 && SelectedTrackIndex < NotifyTrackIndices.Num());
    if (!bCanDelete)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("[-] Delete Track"))
    {
        if (bCanDelete)
        {
            NotifyTrackIndices.RemoveAt(SelectedTrackIndex);
            SelectedTrackIndex = -1;
        }
    }

    if (!bCanDelete)
    {
        ImGui::EndDisabled();
    }

    // 드롭다운 메뉴
    if (ImGui::BeginPopup("AddNotifyTrackMenu"))
    {
        if (ImGui::MenuItem("Add Notify Track"))
        {
            NotifyTrackIndices.Add(NextNotifyTrackNumber);
            NextNotifyTrackNumber++;
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ============================================================
    // 2. 스크롤 가능한 트랙 영역 (Notify + Timeline 함께 스크롤)
    // ============================================================
    // 부모 스크롤 영역 (실제 스크롤바를 표시)
    ImGui::BeginChild("ScrollableTracks", ImVec2(0, ScrollableHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
        // 실제 트랙 개수
        int32 ActualTrackCount = NotifyTrackIndices.Num();

        // 화면을 채우기 위한 최소 트랙 개수 계산
        int32 MinVisibleTracks = std::max(1, (int32)std::ceil(ScrollableHeight / RowHeight));

        // 화면 표시용 트랙 개수 (최소 MinVisibleTracks개는 표시)
        int32 DisplayTrackCount = std::max(ActualTrackCount, MinVisibleTracks);

        // 실제 스크롤 가능한 콘텐츠 높이 (실제 트랙 개수만큼만)
        float ScrollContentHeight = RowHeight * std::max(ActualTrackCount, 1);

        // 화면 표시용 높이 (빈 공간 포함)
        float DisplayHeight = RowHeight * DisplayTrackCount;

        // 부모의 현재 스크롤 위치 가져오기
        float ScrollY = ImGui::GetScrollY();

        // 좌측 Notify 컬럼과 우측 Timeline 컬럼을 같이 렌더링
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // 화면 위치 고정을 위해 SetCursorScreenPos 사용
        ImVec2 BasePos = ImGui::GetCursorScreenPos();

        // 좌측: Notify 트랙 번호 컬럼 (고정 위치, 스크롤 오프셋 적용)
        ImGui::SetCursorScreenPos(ImVec2(BasePos.x, BasePos.y));
        ImGui::BeginChild("NotifyColumn", ImVec2(NotifyColumnWidth, DisplayHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetCursorPosY(-ScrollY);
            RenderNotifyTrackColumn(NotifyColumnWidth, RowHeight, DisplayTrackCount);
        }
        ImGui::EndChild();

        // 우측: Timeline 컬럼 (고정 위치, 스크롤 오프셋 적용)
        ImGui::SetCursorScreenPos(ImVec2(BasePos.x + NotifyColumnWidth, BasePos.y));
        ImGui::BeginChild("TimelineColumn", ImVec2(TimelineColumnWidth, DisplayHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetCursorPosY(-ScrollY);
            RenderTimelineColumn(TimelineColumnWidth, RowHeight, DisplayTrackCount);
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();

        // 더미 아이템으로 스크롤 가능한 영역 설정 (실제 트랙 개수만큼만)
        ImGui::Dummy(ImVec2(0, ScrollContentHeight));
    }
    ImGui::EndChild();

    // ============================================================
    // 3. Playback Controls (하단 고정, 스크롤 안됨)
    // ============================================================
    ImGui::Separator();
    RenderPlaybackControls();
}

void SAnimSequenceViewerWindow::RenderNotifyTrackColumn(float ColumnWidth, float RowHeight, int32 VisibleTrackCount)
{
    // Notify 트랙 번호 표시 (좌측 컬럼)
    for (int i = 0; i < NotifyTrackIndices.Num(); i++)
    {
        int32 TrackNumber = NotifyTrackIndices[i];
        char Label[32];
        sprintf_s(Label, "%d", TrackNumber);

        // 트랙 선택 가능
        bool bSelected = (i == SelectedTrackIndex);
        if (ImGui::Selectable(Label, bSelected, 0, ImVec2(0, RowHeight)))
        {
            SelectedTrackIndex = i;
        }

        // 호버 감지
        if (ImGui::IsItemHovered())
        {
            HoveredTrackIndex = i;
        }
    }

    // 트랙이 없으면 안내 메시지
    if (NotifyTrackIndices.Num() == 0)
    {
        ImGui::TextDisabled("No tracks");
        ImGui::TextDisabled("Click [+]");
    }
}

void SAnimSequenceViewerWindow::RenderTimelineColumn(float ColumnWidth, float RowHeight, int32 VisibleTrackCount)
{
    // Timeline 영역 계산
    TimelineWidth = ColumnWidth - 20.0f;
    float RulerHeight = 35.0f; // 하단 프레임 눈금 영역
    float TotalHeight = (VisibleTrackCount * RowHeight) + RulerHeight;

    ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    ImVec2 CanvasSize(TimelineWidth, TotalHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // 타임라인 배경
    ImVec4 bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    DrawList->AddRectFilled(CanvasPos,
        ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
        ImGui::ColorConvertFloat4ToU32(bgColor));

    // 각 트랙별 구분선 그리기
    for (int i = 0; i <= VisibleTrackCount; i++)
    {
        float YPos = CanvasPos.y + (i * RowHeight);
        DrawList->AddLine(
            ImVec2(CanvasPos.x, YPos),
            ImVec2(CanvasPos.x + CanvasSize.x, YPos),
            IM_COL32(80, 80, 80, 255), 1.0f);
    }

    // 적응형 눈금 간격 계산
    int targetRulerCount = 10;
    int FrameInterval = (TotalFrames > 0) ? (TotalFrames / targetRulerCount) : 10;

    if (FrameInterval <= 5)
        FrameInterval = 5;
    else if (FrameInterval <= 10)
        FrameInterval = 10;
    else if (FrameInterval <= 20)
        FrameInterval = 20;
    else if (FrameInterval <= 30)
        FrameInterval = 30;
    else if (FrameInterval <= 50)
        FrameInterval = 50;
    else if (FrameInterval <= 100)
        FrameInterval = 100;
    else
        FrameInterval = ((FrameInterval + 99) / 100) * 100;

    // 프레임 눈금 그리기 (세로선)
    for (int frame = 0; frame <= TotalFrames; frame += FrameInterval)
    {
        float Time = FrameToTime(frame);
        float XPos = CanvasPos.x + TimeToPixel(Time);

        // 큰 눈금선 (전체 높이)
        DrawList->AddLine(
            ImVec2(XPos, CanvasPos.y),
            ImVec2(XPos, CanvasPos.y + CanvasSize.y),
            IM_COL32(100, 100, 100, 255), 1.0f);

        // 프레임 번호 표시 (하단)
        char Label[16];
        sprintf_s(Label, "%d", frame);
        DrawList->AddText(
            ImVec2(XPos - 10, CanvasPos.y + CanvasSize.y - 30),
            IM_COL32(200, 200, 200, 255), Label);
    }

    // 작은 눈금
    int smallInterval = (FrameInterval >= 50) ? (FrameInterval / 5) : (FrameInterval / 2);
    if (smallInterval > 0)
    {
        for (int frame = 0; frame <= TotalFrames; frame += smallInterval)
        {
            if (frame % FrameInterval == 0) continue;

            float Time = FrameToTime(frame);
            float XPos = CanvasPos.x + TimeToPixel(Time);

            DrawList->AddLine(
                ImVec2(XPos, CanvasPos.y),
                ImVec2(XPos, CanvasPos.y + CanvasSize.y - RulerHeight),
                IM_COL32(60, 60, 60, 255), 1.0f);
        }
    }

    // 재생 헤드 (Playhead)
    float PlayheadX = CanvasPos.x + TimeToPixel(CurrentTime);

    // 재생 헤드 라인
    DrawList->AddLine(
        ImVec2(PlayheadX, CanvasPos.y),
        ImVec2(PlayheadX, CanvasPos.y + CanvasSize.y),
        IM_COL32(255, 100, 100, 255), 3.0f);

    // 재생 헤드 상단 삼각형
    ImVec2 triangle[3] = {
        ImVec2(PlayheadX, CanvasPos.y),
        ImVec2(PlayheadX - 6, CanvasPos.y + 10),
        ImVec2(PlayheadX + 6, CanvasPos.y + 10)
    };
    DrawList->AddTriangleFilled(triangle[0], triangle[1], triangle[2],
        IM_COL32(255, 100, 100, 255));

    // 타임라인 클릭/드래그 감지
    ImGui::SetCursorScreenPos(CanvasPos);
    ImGui::InvisibleButton("TimelineButton", CanvasSize);

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
    {
        bIsDraggingPlayhead = true;
        ImVec2 MousePos = ImGui::GetMousePos();
        float ClickX = MousePos.x - CanvasPos.x;
        CurrentTime = PixelToTime(ClickX);
        CurrentFrame = TimeToFrame(CurrentTime);
        bIsPlaying = false;
    }
    else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 MousePos = ImGui::GetMousePos();
        float ClickX = MousePos.x - CanvasPos.x;
        CurrentTime = PixelToTime(ClickX);
        CurrentFrame = TimeToFrame(CurrentTime);
        bIsPlaying = false;
    }
    else
    {
        bIsDraggingPlayhead = false;
    }

    // 커서 이동
    ImGui::SetCursorScreenPos(ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y + 5));
    ImGui::Spacing();
}

