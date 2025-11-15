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

	// 처음 한번만 윈도우 위치/크기 설정
	if (!bInitialPlacementDone)
	{
		ImGui::SetNextWindowPos(ImVec2(100, 100));
		ImGui::SetNextWindowSize(ImVec2(900, 600));
		bInitialPlacementDone = true;
	}
	// 윈도우 시작
	if (ImGui::Begin("Animation Sequence Viewer", &bIsOpen))
	{
		// Step 1: 단순 텍스트만 표시
		ImGui::Text("Animation Sequence Viewer - Step 1: Empty Window");
		ImGui::Separator();
		ImGui::Text("This is a placeholder. Features will be added step by step.");
	}
	ImGui::End();

	// 윈도우가 닫히면 정리
	if (!bIsOpen)
	{
		// USlateManager에 알림
	}
}	

void SAnimSequenceViewerWindow::OnUpdate(float DeltaSeconds)
{
}
