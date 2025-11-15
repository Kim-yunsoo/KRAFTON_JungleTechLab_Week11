#include "pch.h"
#include "AnimSequenceViewerWindow.h"

SAnimSequenceViewerWindow::SAnimSequenceViewerWindow()
{
}

SAnimSequenceViewerWindow::~SAnimSequenceViewerWindow()
{
}

bool SAnimSequenceViewerWindow::Initialize()
{
	return true;
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

        ImGui::BeginChild("Timeline", ImVec2(TotalWidth, TimelineHeight), true);
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
}

void SAnimSequenceViewerWindow::RenderAnimationList()
{
    ImGui::Text("Animation List");
    ImGui::Separator();
    ImGui::Spacing();

    // Step 2: 더미 데이터로 목록 표시
    ImGui::Text("Available Animations:");
    ImGui::Spacing();

    // 임시 하드코딩된 애니메이션 목록
    const char* DummyAnims[] = {
        "MM_Idle",
        "MM_Walk",
        "MM_Run",
        "MM_Jump"
    };

    for (int i = 0; i < 4; i++)
    {
        bool bIsSelected = (SelectedAnimIndex == i);

        if (ImGui::Selectable(DummyAnims[i], bIsSelected))
        {
            SelectedAnimIndex = i;
            // Step 4+에서 실제 애니메이션 로드
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
    if (SelectedAnimIndex >= 0)
    {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
            "Selected: %s", DummyAnims[SelectedAnimIndex]);
    }
    else
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "No animation selected");
    }
}

void SAnimSequenceViewerWindow::RenderInfoPanel()
{
    ImGui::Text("Animation Information");
    ImGui::Separator();
    ImGui::Spacing();

    // Step 2: 플레이스홀더 정보
    if (SelectedAnimIndex >= 0)
    {
        // 임시 정보 표시
        ImGui::Text("Name: MM_Animation_%d", SelectedAnimIndex);
        ImGui::Text("Length: 2.50 seconds (placeholder)");
        ImGui::Text("Frames: 75 frames (placeholder)");
        ImGui::Text("FPS: 30 (placeholder)");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 상세 정보
        ImGui::Text("Details:");
        ImGui::BulletText("File Path: (not loaded)");
        ImGui::BulletText("Bone Tracks: 0");
        ImGui::BulletText("Notify Events: 0");
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
            // TODO: 재생/일시정지 토글
            bIsPlaying = !bIsPlaying;
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
