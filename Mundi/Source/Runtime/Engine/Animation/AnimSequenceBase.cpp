#include "pch.h"
#include "AnimSequenceBase.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UAnimSequenceBase)

void UAnimSequenceBase::AddNotify(const FAnimNotifyEvent& Notify)
{
    FString TestSrt = Notify.NotifyName.ToString();
    // 이진 탐색을 통해 추가할 지점을 찾아 삽입합니다.
    auto InsertPos = std::lower_bound(Notifies.begin(), Notifies.end(), Notify,
        [](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
        {
            return A.TriggerTime < B.TriggerTime;
        });

    Notifies.insert(InsertPos, Notify);
}

void UAnimSequenceBase::SetNotifies(const TArray<FAnimNotifyEvent>& InNotifies)
{
    Notifies = InNotifies;
    SortNotifies();
}

void UAnimSequenceBase::RemoveNotifiesByName(const FName& InName)
{
    if (Notifies.empty()) { return; }

    Notifies.erase(std::remove_if(Notifies.begin(),Notifies.end(),
        [&InName](const FAnimNotifyEvent& Notify)
        {
            return Notify.NotifyName == InName;
        }),
        Notifies.end());
}

// 애니메이션의 특정 구간에서 노티파이 이벤트 목록들을 수집해서 반환합니다
// ============================================================================
// UAnimSequenceBase::GetAnimNotifiesInRange
// 
// 역할:
//   애니메이션이 StartTime → EndTime 구간 동안 "지나간" AnimNotify들을 찾아
//   OutNotifies 배열에 채워 넣는다.
//
// 왜 필요한가?
//   애니메이션은 매 프레임 시간(dt)만큼 재생되므로,
//   이전 프레임 위치(StartTime)에서 이번 프레임 위치(EndTime)으로
//   이동하는 동안 Notify(예: 발소리, 몽타주 이벤트)가
//   이 구간을 통과했는지를 체크해야 한다.
//
// 주요 특징:
//   • Notify.TriggerTime > StartTime && <= EndTime 조건으로
//     "이번 프레임에서 막 지나간" Notify만 가져온다
//   • 루프 애니메이션의 경우 Start > End(예: 0.95 → 0.02) 상황을 처리
//   • 오염 방지를 위해 OutNotifies.clear()
// ============================================================================

void UAnimSequenceBase::GetAnimNotifiesInRange(float StartTime, float EndTime, TArray<FAnimNotifyEvent>& OutNotifies) const
{
    OutNotifies.clear(); // 이전 프레임 데이터 오염 방지

    if (Notifies.empty()) { return; }

    // --------------------------
    // 1) 정방향 재생 (일반 상황)
    //    예: Start = 0.3 → End = 0.45
    // --------------------------
    if (StartTime <= EndTime)
    {
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            // Notify.TriggerTime 이 해당 프레임에서 "막 지남"을 의미
            // > 를 사용하는 이유: StartTime == NotifyTime 인 프레임에서
            //                      중복 발생을 방지
            if (Notify.TriggerTime > StartTime && Notify.TriggerTime <= EndTime)
            {
                OutNotifies.Add(Notify);
            }
        }
    }
    // --------------------------
    // 2) 루프 상황 (애니메이션이 끝에서 다시 0으로 돌아감)
    //    예: Start=0.95, End=0.04
    //
    //    처리 방식:
    //      A) [StartTime ~ SequenceEnd] 에서 지나간 Notify
    //      B) [0 ~ EndTime] 에서 지나간 Notify
    // --------------------------
    else
    {
        // A) 마지막 구간: Start → TotalPlayLength
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            if (Notify.TriggerTime > StartTime && Notify.TriggerTime <= TotalPlayLength)
            {
                OutNotifies.Add(Notify);
            }
        }

        // B) 루프해서 다시 0 → EndTime
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            if (Notify.TriggerTime >= 0.0f && Notify.TriggerTime <= EndTime)
            {
                OutNotifies.Add(Notify);
            }
        }
    }
}

void UAnimSequenceBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "TotalPlayLength", TotalPlayLength, 0.f, false);
        FJsonSerializer::ReadFloat(InOutHandle, "PlayRate", PlayRate, 1.f, false);
        FJsonSerializer::ReadBool(InOutHandle, "bLoop", bLoop, false, false);

        JSON NotifyArray;
        if (FJsonSerializer::ReadArray(InOutHandle, "Notifies", NotifyArray, nullptr, false))
        {
            Notifies.clear();
            for (size_t Idx = 0; Idx < NotifyArray.size(); ++Idx)
            {
                const JSON& NotifyJson = NotifyArray.at(static_cast<unsigned>(Idx));
                if (NotifyJson.JSONType() != JSON::Class::Object) { continue; }

                FAnimNotifyEvent Notify;
                FJsonSerializer::ReadFloat(NotifyJson, "TriggerTime", Notify.TriggerTime, 0.f, false);
                FJsonSerializer::ReadFloat(NotifyJson, "Duration", Notify.Duration, 0.f, false);

                FString NotifyNameStr;
                if (FJsonSerializer::ReadString(NotifyJson, "NotifyName", NotifyNameStr, "", false))
                {
                    Notify.NotifyName = FName(NotifyNameStr);
                }

                Notifies.Add(Notify);
            }
            SortNotifies();
        }
        else
        {
            Notifies.clear();
        }
    }
    else
    {
        InOutHandle["TotalPlayLength"] = TotalPlayLength;
        InOutHandle["PlayRate"] = PlayRate;
        InOutHandle["bLoop"] = bLoop;

        JSON NotifyArray = JSON::Make(JSON::Class::Array);
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            JSON NotifyJson = JSON::Make(JSON::Class::Object);
            NotifyJson["TriggerTime"] = Notify.TriggerTime;
            NotifyJson["Duration"] = Notify.Duration;
            NotifyJson["NotifyName"] = Notify.NotifyName.ToString().c_str();
            NotifyArray.append(NotifyJson);
        }
        InOutHandle["Notifies"] = NotifyArray;
    }
}
