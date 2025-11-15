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
        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float totalWidth = contentAvail.x;
        float totalHeight = contentAvail.y;

        // 패널 너비 계산
        float leftWidth = totalWidth * 0.3f;    // 30%
        float rightWidth = totalWidth * 0.7f;   // 70%
        float timelineHeight = 120.0f;

        // ============================================================
        // 상단 영역 (좌측 + 우측)
        // ============================================================

        // 좌측 패널 - 애니메이션 목록
        ImGui::BeginChild("AnimList", ImVec2(leftWidth, totalHeight - timelineHeight), true);
        {
            RenderAnimationList();  // 별도 메서드로 분리
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // 우측 패널 - 애니메이션 정보
        ImGui::BeginChild("InfoPanel", ImVec2(rightWidth, totalHeight - timelineHeight), true);
        {
            RenderInfoPanel();  // 별도 메서드로 분리
        }
        ImGui::EndChild();

        // ============================================================
        // 하단 영역 - 타임라인
        // ============================================================

        ImGui::BeginChild("Timeline", ImVec2(totalWidth, timelineHeight), true);
        {
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
    const char* dummyAnims[] = {
        "MM_Idle",
        "MM_Walk",
        "MM_Run",
        "MM_Jump"
    };

    for (int i = 0; i < 4; i++)
    {
        bool isSelected = (SelectedAnimIndex == i);

        if (ImGui::Selectable(dummyAnims[i], isSelected))
        {
            SelectedAnimIndex = i;
            // Step 4+에서 실제 애니메이션 로드
        }

        if (isSelected)
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
            "Selected: %s", dummyAnims[SelectedAnimIndex]);
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
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = 40.0f;
    float spacing = 8.0f;
    float totalWidth = (buttonWidth * 5) + (spacing * 4); // 5개 버튼 + 4개 간격
    float startX = (windowWidth - totalWidth) * 0.5f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

    // 재생 컨트롤 버튼들
    ImGui::BeginGroup();
    {
        // Previous Frame 버튼
        if (ImGui::Button("|<<", ImVec2(buttonWidth, 30)))
        {
            // TODO: 이전 프레임으로 이동
            if (CurrentFrame > 0)
                CurrentFrame--;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Previous Frame");

        ImGui::SameLine(0, spacing);

        // Play/Pause 버튼
        const char* playButtonText = bIsPlaying ? "||" : ">";
        if (ImGui::Button(playButtonText, ImVec2(buttonWidth, 30)))
        {
            // TODO: 재생/일시정지 토글
            bIsPlaying = !bIsPlaying;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(bIsPlaying ? "Pause" : "Play");

        ImGui::SameLine(0, spacing);

        // Stop 버튼
        if (ImGui::Button("[]", ImVec2(buttonWidth, 30)))
        {
            // TODO: 정지 (처음으로)
            bIsPlaying = false;
            CurrentFrame = 0;
            CurrentTime = 0.0f;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stop");

        ImGui::SameLine(0, spacing);

        // Next Frame 버튼
        if (ImGui::Button(">>|", ImVec2(buttonWidth, 30)))
        {
            // TODO: 다음 프레임으로 이동
            if (CurrentFrame < TotalFrames - 1)
                CurrentFrame++;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Next Frame");

        ImGui::SameLine(0, spacing);

        // Loop Toggle 버튼
        ImVec4 loopColor = bLooping ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, loopColor);
        if (ImGui::Button("Loop", ImVec2(buttonWidth, 30)))
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
