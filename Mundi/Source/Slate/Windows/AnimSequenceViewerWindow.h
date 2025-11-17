#pragma once
#include "SWindow.h"
#include "AnimSequence.h"
#include "AnimSequenceBase.h"

// Forward declarations
class ViewerState;
class UWorld;
struct ID3D11Device;

/**
* @brief 애니메이션 시퀀스 뷰어 윈도우
* - 상단: 3D 프리뷰 뷰포트 (자체 ViewerState)
* - 하단 좌측: Notify 트랙 (Add Notify 버튼)
* - 하단 중앙: 타임라인 UI (재생 컨트롤, 프레임 눈금)
* - 하단 우측 상단: Animation Info
* - 하단 우측 하단: Animation List
*/
class SAnimSequenceViewerWindow : public SWindow
{
public:
	SAnimSequenceViewerWindow();
	virtual ~SAnimSequenceViewerWindow();

	bool Initialize(UWorld* InWorld, ID3D11Device* InDevice);

	// 애니메이션 로드
	void LoadAnimSquence(UAnimSequence* Sequence);

	// 윈도우 상태
	bool IsOpen() const { return bIsOpen; }
	void Close() { bIsOpen = false; }

	// SWindow 오버라이드
	virtual void OnRender() override;
	virtual void OnUpdate(float DeltaSeconds) override;
	//virtual void OnMouseMove(FVector2D MousePos) override;
	//virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
	//virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

private:
	// === UI 렌더링 메서드 ===

	/** 상단: 3D 프리뷰 뷰포트 */
	void RenderPreviewViewport(float Height);
	/** 하단 좌측: Notify 트랙 패널 */
	void RenderNotifyTrackPanel();
	/** 하단 중앙: 재생 컨트롤 */
	void RenderPlaybackControls();
	/** 하단 중앙: 타임라인 (프레임 눈금, 재생 헤드, Notify 마커) */
	void RenderTimeline();
	/** 하단 우측 상단: 애니메이션 정보 패널 */
	void RenderInfoPanel();
	/** 하단 우측 하단: 애니메이션 목록 */
	void RenderAnimationList();

private:
	// === 타임라인 UI 헬퍼 메서드 ===
	/** 시간 → 화면 X좌표 변환 */
	float TimeToPixel(float Time) const;
	
	/** 화면 X좌표 → 시간 변환 */
	float PixelToTime(float PixelX) const;
	
	/** 프레임 → 시간 변환 */
	float FrameToTime(int32 Frame) const;
	
	/** 시간 → 프레임 변환 */
	int32 TimeToFrame(float Time) const;

private:
	// === 자체 프리뷰 시스템 ===
	ViewerState* PreviewState = nullptr;
	UWorld* World = nullptr;
	ID3D11Device* Device = nullptr;

	// === 레이아웃 비율 ===
	float TopPreviewHeight = 0.6f;      // 상단 프리뷰 60%
	float BottomPanelHeight = 0.4f;     // 하단 전체 40%

	float LeftNotifyWidth = 0.30f;      // 좌측 Notify 30%
	float CenterTimelineWidth = 0.40f;  // 중앙 Timeline 40%
	float RightPanelWidth = 0.3f;       // 우측 Info+List 30%

	float RightTopInfoHeight = 0.4f;    // 우측 상단 Info 40%
	float RightBottomListHeight = 0.6f; // 우측 하단 List 60%

	// === Notify 상태 ===
	int32 HoveredNotifyIndex = -1;
	int32 SelectedNotifyIndex = -1;

	// === Notify 트랙 관리 ===
	TArray<int32> NotifyTrackIndices; // 추가된 노티파이 트랙 번호 목록 (1, 2, 3...)
	int32 NextNotifyTrackNumber = 1;  // 다음 트랙 번호
	int32 HoveredTrackIndex = -1;     // 마우스 오버된 트랙 인덱스
	int32 RightClickedTrackIndex = -1; // 우클릭된 트랙 인덱스

private:
	// 애니메이션 데이터
	TArray<FString> AvailableAnimationPaths; // 애니메이션 파일 경로 목록
	int32 SelectedAnimIndex = -1;
	UAnimSequenceBase* CurrentSequence; // 현재 보고 있는 애니메이션 시퀀스
	
	// 재생 상태
	float CurrentTime = 0.0f;
	float PlayLength = 5.0f; // 임시 기본값
	float PlayRate = 1.0f;
	bool bIsPlaying = false;
	bool bLooping = true;
	int32 CurrentFrame = 0;
	int32 TotalFrames = 150; // 임시 기본값
	
	// UI 상태
	float TimelineWidth = 800.0f;
	bool bIsDraggingPlayhead = false; // 재생 헤드 드래그 중

	bool bInitialPlacementDone = false;
	bool bIsOpen = true;


};

