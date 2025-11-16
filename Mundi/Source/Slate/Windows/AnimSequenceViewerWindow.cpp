#include "pch.h"
#include "AnimSequenceViewerWindow.h"
#include "USlateManager.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"

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
	// 더미 시퀀스 정리
	if (CurrentSequence)
	{
		CurrentSequence = nullptr;
	}
}

bool SAnimSequenceViewerWindow::Initialize()
{
	return true;
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
}

//void SAnimSequenceViewerWindow::LoadAnimSquence(UAnimSequence* Sequence)
//{
//	
//}

void SAnimSequenceViewerWindow::OnRender()
{
	if (!bIsOpen) return;

	// 처음 한 번만 윈도우 위치/크기 설정
	if (!bInitialPlacementDone)
	{
		ImGui::SetNextWindowPos(ImVec2(100, 100));
		ImGui::SetNextWindowSize(ImVec2(1000, 700));  // 크기 조금 키움
		bInitialPlacementDone = true;
	}

    // 윈도우 시작 (사용자가 X버튼 누르면 bIsOpen이 false가 됨)
    if (ImGui::Begin("Animation Sequence Viewer", &bIsOpen))
    {
        ImVec2 ContentAvail = ImGui::GetContentRegionAvail();
        float TotalWidth = ContentAvail.x;
        float TotalHeight = ContentAvail.y;

        // 패널 너비 계산
        float LeftWidth = TotalWidth * 0.3f;    // 30%
        float RightWidth = TotalWidth * 0.7f;   // 70%
        float TimelineHeight = 200.0f;

        // ============================================================
        // 상단 영역 (좌측 + 우측)
        // ============================================================

        // 좌측 패널 - 애니메이션 목록
        ImGui::BeginChild("AnimList", ImVec2(LeftWidth, TotalHeight - TimelineHeight), true);
        {
            RenderAnimationList();  // 별도 메서드로 분리
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // 우측 패널 - 애니메이션 정보
        ImGui::BeginChild("InfoPanel", ImVec2(RightWidth, TotalHeight - TimelineHeight), true);
        {
            RenderInfoPanel();  // 별도 메서드로 분리
        }
        ImGui::EndChild();

        // ============================================================
        // 하단 영역 - 타임라인
        // ============================================================

        ImGui::BeginChild("Timeline", ImVec2(TotalWidth, 280), true);  // Notify 트랙 추가로 높이 증가
        {
            // 타임라인 렌더링
            RenderTimeline();

            ImGui::Separator();
            
            // 재생 컨트롤
            RenderPlaybackControls();
        }
        ImGui::EndChild();
    }
    ImGui::End();

    // 윈도우가 닫히면 정리
    if (!bIsOpen)
    {
        // USlateManager에 알림 (나중에 구현)
    }
}	

void SAnimSequenceViewerWindow::OnUpdate(float DeltaSeconds)
{
    UE_LOG("[AnimSequenceViewer] OnUpdate");
    // 타임라인 자동 재생
    if (bIsPlaying && CurrentSequence)
    {
        // 시간 증가
        CurrentTime += DeltaSeconds * PlayRate;

        // 디버그: 1초마다 로그 출력
        static float debugTimer = 0.0f;
        debugTimer += DeltaSeconds;
        if (debugTimer >= 1.0f)
        {
            UE_LOG("[AnimSequenceViewer] Playing - Time: %.2f / %.2f, Frame: %d / %d",
                CurrentTime, PlayLength, CurrentFrame, TotalFrames);
            debugTimer = 0.0f;
        }

        // 루프 처리
        if (CurrentTime > PlayLength)
        {
            if(bLooping)
            {
                // 루프면 처음으로 되돌림
                CurrentTime = fmod(CurrentTime, PlayLength);
            }
            else
            {
                // 루프 아님: 끝에서 정지
                CurrentTime = PlayLength;
                bIsPlaying = false;
            }
        }
        // 프레임 업데이트
        CurrentFrame = TimeToFrame(CurrentTime);
    }
	// Step 6: SkeletalViewer에 현재 시간 반영
	ApplyToSkeletalViewer();
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
                UE_LOG("[AnimSequenceViewer] Play button clicked. bIsPlaying: %s", bIsPlaying ? "true" : "false");
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
            // TODO: 정지 (처음으로)
            bIsPlaying = false;
            CurrentFrame = 0;
            CurrentTime = 0.0f;
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
    ImGui::SliderFloat("Playback Speed", &PlayRate, 0.1f, 2.0f, "%.2fx");
}

void SAnimSequenceViewerWindow::RenderTimeline()
{
    ImGui::Text("Timeline");
    ImGui::Spacing();

    // Step 5: Notify 트랙 먼저 렌더링
    RenderNotifyMarkers();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 타임라인 영역 크기 계산
    TimelineWidth = ImGui::GetContentRegionAvail().x - 20.0f;
    float TimelineHeight = 60.0f;

    ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
    ImVec2 CanvasSize(TimelineWidth, TimelineHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // 타임라인 배경
    ImVec4 bgColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    DrawList->AddRectFilled(CanvasPos,
        ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
        ImGui::ColorConvertFloat4ToU32(bgColor));

    // 프레임 눈금 그리기
    int FrameInterval = 30; // 30프레임마다 큰 눈금
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

    // 작은 눈금 (5프레임마다)
    for (int frame = 0; frame <= TotalFrames; frame += 5)
    {
        if (frame % FrameInterval == 0) continue; // 큰 눈금은 건너뛰기

        float Time = FrameToTime(frame);
        float XPos = CanvasPos.x + TimeToPixel(Time);

        DrawList->AddLine(
            ImVec2(XPos, CanvasPos.y + CanvasSize.y - 8),
            ImVec2(XPos, CanvasPos.y + CanvasSize.y),
            IM_COL32(100, 100, 100, 255), 1.0f);
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

void SAnimSequenceViewerWindow::RenderNotifyMarkers()
{
	ImGui::Text("Notify Track");
	ImGui::Spacing();

	// Notify 트랙 영역
	float TrackWidth = TimelineWidth;
	float TrackHeight = 50.0f;

	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize(TrackWidth, TrackHeight);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// 트랙 배경
	DrawList->AddRectFilled(CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		IM_COL32(40, 40, 45, 255));

	// 트랙 테두리
	DrawList->AddRect(CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		IM_COL32(80, 80, 90, 255), 0.0f, 0, 1.0f);

	// 시퀀스가 없으면 빈 트랙만 표시
	if (!CurrentSequence)
	{
		ImGui::SetCursorScreenPos(ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y + 5));
		ImGui::Spacing();
		return;
	}

	// 실제 Notify 데이터 가져오기
	const TArray<FAnimNotifyEvent>& Notifies = CurrentSequence->GetNotifies();

	// Notify 마커 그리기
	HoveredNotifyIndex = -1;

	for (int i = 0; i < Notifies.size(); i++)
	{
		const FAnimNotifyEvent& Notify = Notifies[i];
		float XPos = CanvasPos.x + TimeToPixel(Notify.TriggerTime);

		// Duration이 있는 경우 (Notify State) - 사각형으로 표시
		if (Notify.Duration > 0.0f)
		{
			float EndX = CanvasPos.x + TimeToPixel(Notify.GetEndTime());
			float Width = EndX - XPos;

			ImU32 RectColor = (i == SelectedNotifyIndex)
				? IM_COL32(255, 180, 100, 120)  // 선택됨
				: IM_COL32(100, 150, 255, 80);   // 기본

			// Duration 사각형
			DrawList->AddRectFilled(
				ImVec2(XPos, CanvasPos.y + 5),
				ImVec2(EndX, CanvasPos.y + TrackHeight - 5),
				RectColor, 3.0f);

			DrawList->AddRect(
				ImVec2(XPos, CanvasPos.y + 5),
				ImVec2(EndX, CanvasPos.y + TrackHeight - 5),
				IM_COL32(150, 200, 255, 255), 3.0f, 0, 2.0f);
		}

		// 마커 위치 (다이아몬드 모양)
		ImVec2 Center(XPos, CanvasPos.y + TrackHeight * 0.5f);
		float Size = 8.0f;

		ImVec2 Diamond[4] = {
			ImVec2(Center.x, Center.y - Size),        // 위
			ImVec2(Center.x + Size, Center.y),        // 오른쪽
			ImVec2(Center.x, Center.y + Size),        // 아래
			ImVec2(Center.x - Size, Center.y)         // 왼쪽
		};

		// 선택/호버 상태에 따른 색상
		ImU32 MarkerColor;
		if (i == SelectedNotifyIndex)
			MarkerColor = IM_COL32(255, 200, 100, 255); // 주황색 (선택됨)
		else if (i == HoveredNotifyIndex)
			MarkerColor = IM_COL32(150, 200, 255, 255); // 밝은 파란색 (호버)
		else
			MarkerColor = IM_COL32(100, 150, 255, 255); // 기본 파란색

		// 다이아몬드 마커 그리기
		DrawList->AddQuadFilled(Diamond[0], Diamond[1], Diamond[2], Diamond[3], MarkerColor);
		DrawList->AddQuad(Diamond[0], Diamond[1], Diamond[2], Diamond[3],
			IM_COL32(255, 255, 255, 200), 1.5f);

		// Notify 이름 텍스트
		ImVec2 TextPos(XPos + Size + 4, CanvasPos.y + TrackHeight * 0.5f - 8);
		DrawList->AddText(TextPos, IM_COL32(220, 220, 220, 255),
			Notify.NotifyName.ToString().c_str());

		// 마우스 호버 감지
		ImVec2 MousePos = ImGui::GetMousePos();
		float Dist = sqrtf(
			(MousePos.x - Center.x) * (MousePos.x - Center.x) +
			(MousePos.y - Center.y) * (MousePos.y - Center.y)
		);

		if (Dist < Size + 5.0f)
		{
			HoveredNotifyIndex = i;

			// 툴팁 표시
			if (Notify.Duration > 0.0f)
			{
				ImGui::SetTooltip("%s\nTime: %.2fs - %.2fs (Duration: %.2fs)\nFrame: %d - %d",
					Notify.NotifyName.ToString().c_str(),
					Notify.TriggerTime,
					Notify.GetEndTime(),
					Notify.Duration,
					TimeToFrame(Notify.TriggerTime),
					TimeToFrame(Notify.GetEndTime()));
			}
			else
			{
				ImGui::SetTooltip("%s\nTime: %.2fs (Frame: %d)",
					Notify.NotifyName.ToString().c_str(),
					Notify.TriggerTime,
					TimeToFrame(Notify.TriggerTime));
			}
		}
	}

	// 트랙 클릭 감지
	ImGui::SetCursorScreenPos(CanvasPos);
	ImGui::InvisibleButton("NotifyTrackButton", CanvasSize);

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		if (HoveredNotifyIndex >= 0)
		{
			// Notify 마커 클릭
			SelectedNotifyIndex = HoveredNotifyIndex;
		}
		else
		{
			// 빈 공간 클릭 (선택 해제)
			SelectedNotifyIndex = -1;
		}
	}

	// 우클릭 컨텍스트 메뉴
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		if (HoveredNotifyIndex >= 0)
		{
			ImGui::OpenPopup("NotifyContextMenu");
		}
	}

	// 컨텍스트 메뉴
	if (ImGui::BeginPopup("NotifyContextMenu"))
	{
		if (ImGui::MenuItem("Delete Notify"))
		{
			if (SelectedNotifyIndex >= 0 && SelectedNotifyIndex < Notifies.size())
			{
				// UAnimSequenceBase의 RemoveNotifiesByName 사용
				CurrentSequence->RemoveNotifiesByName(Notifies[SelectedNotifyIndex].NotifyName);
				SelectedNotifyIndex = -1;
			}
		}
		ImGui::EndPopup();
	}

	// 커서 이동
	ImGui::SetCursorScreenPos(ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y + 5));
	ImGui::Spacing();
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

// ============================================================
// Step 6: SkeletalViewer 연동 (나중에 연동하지 않고 별도 프리뷰로 만들 예정)
// ============================================================

SSkeletalMeshViewerWindow* SAnimSequenceViewerWindow::GetSkeletalViewer()
{
	USlateManager& SlateManager = USlateManager::GetInstance();
	return SlateManager.GetSkeletalMeshViewer();
}

ViewerState* SAnimSequenceViewerWindow::GetViewerState()
{
	SSkeletalMeshViewerWindow* SkeletalViewer = GetSkeletalViewer();
	if (!SkeletalViewer)
		return nullptr;

	return SkeletalViewer->GetCurrentViewerState();
}

void SAnimSequenceViewerWindow::ApplyToSkeletalViewer()
{
	ViewerState* State = GetViewerState();
	if (!State)
		return;

	// 현재 시간을 SkeletalViewer에 반영
	State->CurrentTime = CurrentTime;
	State->bIsPlaying = bIsPlaying;
	State->PlayRate = PlayRate;
}

