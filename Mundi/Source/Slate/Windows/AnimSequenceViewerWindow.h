#pragma once
#include "SWindow.h"
#include "AnimSequence.h"
/**
* @brief 애니메이션 시퀀스 뷰어 윈도우
* - 타임라인 UI (프레임 눈금, 재생 헤드)
* - 재생 컨트롤 (Play/Pause/Stop, 프레임 이동)
* - Notify 트랙 (마커 표시, 드래그 편집, 추가/삭제)
* - SkeletalViewer와 실시간 연동
*/
class SAnimSequenceViewerWindow : public SWindow
{
public:
	SAnimSequenceViewerWindow();
	virtual ~SAnimSequenceViewerWindow();

	bool Initialize();

	// 애니메이션 로드
	//void LoadAnimSquence(UAnimSequence* Sequence);

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
	
	/** 좌측: 애니메이션 목록 */
	void RenderAnimationList();
	/** 우측: 애니메이션 정보 패널*/
	void RenderInfoPanel();
	/** 재생 컨트롤*/
	void RenderPlaybackControls();
	/** 타임라인 (프레임 눈금, 재생 헤드) */
	void RenderTimeline();
	///** Notify 트랙 (마커, 드래그, 편집) */
	//void RenderNotifyMarkers();

private:
	// === 헬퍼 메서드 ===
	
    /** SkeletalViewer에 현재 시간 반영 (실시간 동기화) */
	//void ApplyToSkeletalViewer();
	
	/** SkeletalViewer 가져오기 */
	//SSkeletalMeshViewerWindow* GetSkeletalViewer();
	
	/** ViewerState 가져오기 */
	//ViewerState* GetViewerState();
	
	/** 시간 → 화면 X좌표 변환 */
	float TimeToPixel(float Time) const;
	
	/** 화면 X좌표 → 시간 변환 */
	float PixelToTime(float PixelX) const;
	
	/** 프레임 → 시간 변환 */
	float FrameToTime(int32 Frame) const;
	
	/** 시간 → 프레임 변환 */
	int32 TimeToFrame(float Time) const;

private:
	//// === Notify 편집 메서드 ===
	//
	///** Notify 마커 히트 테스트 */
	//int32 HitTestNotify(FVector2D MousePos);
	//
	///** Notify 추가 팝업 표시 */
	//void ShowAddNotifyPopup(float TriggerTime);
	//
	///** Notify 삭제 */
	//void DeleteNotify(int32 NotifyIndex);
	//
	///** Notify 컨텍스트 메뉴 */
	//void ShowNotifyContextMenu(int32 NotifyIndex);

private:
	//// 애니메이션 데이터
	//void* CurrentSquence = nullptr; // UAnimSquence* 대신 임시
	//TArray<FString> AvailableAnimationNames; // 임시 목록
	int32 SelectedAnimIndex = -1;
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

