#pragma once
#include "AnimAsset.h"

struct FAnimNotifyEvent
{
    float TriggerTime = 0.f;    // 이벤트가 발생의 시작 지점
    float Duration = 0.f;       // 이벤트 트리거가 유효할 시간
    FName NotifyName;           

    float GetEndTime() const { return TriggerTime + Duration; }
    bool IsWithin(float StartTime, float EndTime) const
    {
        return TriggerTime >= StartTime && TriggerTime <= EndTime;
    }
};

class UAnimSequenceBase : public UAnimAsset
{
public:
    DECLARE_CLASS(UAnimSequenceBase, UAnimAsset);

    UAnimSequenceBase() = default;
    virtual ~UAnimSequenceBase() override = default;

    float GetPlayLength() const override { return TotalPlayLength; }

    const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }
    void SetNotifies(const TArray<FAnimNotifyEvent>& InNotifies);
    void AddNotify(const FAnimNotifyEvent& Notify);
    void RemoveNotifiesByName(const FName& InName);
    void ClearNotifies() { Notifies.clear(); }
    void GetAnimNotifiesInRange(float StartTime, float EndTime, TArray<FAnimNotifyEvent>& OutNotifies) const;

    float GetSequenceLength() const { return TotalPlayLength; }
    void SetSequenceLength(float InLength) { TotalPlayLength = std::max(0.f, InLength); }

    float GetPlayScale() const { return PlayRate; }
    void SetPlayScale(float InPlayRate) { PlayRate = std::max(0.0001f, InPlayRate); }

    bool IsLooping() const { return bLoop; }
    void SetLooping(bool bInLoop) { bLoop = bInLoop; }

    // 공통 메타데이터 직렬화
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    // 내부 헬퍼 함수를 통해서 Notify 배열의 시간대별 정렬을 보장
    void SortNotifies()
    {
        std::sort(Notifies.begin(), Notifies.end(),
            [](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
            {
                return A.TriggerTime < B.TriggerTime;
            });
    }

protected:
    TArray<FAnimNotifyEvent> Notifies;   // 시퀀스에 배치된 노티파이 목록
    float TotalPlayLength = 0.f;         // 전체 재생 시간(초)
    float PlayRate = 1.f;                // 속도 배율 (1.0 = 원래 속도)
    bool bLoop = false;                  // 애니메이션 반복 여부
};
