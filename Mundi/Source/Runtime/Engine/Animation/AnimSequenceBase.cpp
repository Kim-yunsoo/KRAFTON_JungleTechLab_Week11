#include "pch.h"
#include "AnimSequenceBase.h"
#include "JsonSerializer.h"

void UAnimSequenceBase::AddNotify(const FAnimNotifyEvent& Notify)
{
    Notifies.Add(Notify);
    SortNotifies();
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
void UAnimSequenceBase::GetAnimNotifiesInRange(float StartTime, float EndTime, TArray<FAnimNotifyEvent>& OutNotifies) const
{
    OutNotifies.clear(); // 오염 방지

    if (Notifies.empty()) { return; }

    if (StartTime <= EndTime) // 정방향으로 진행하는 경우
    {
        for (const FAnimNotifyEvent& Notify : Notifies)
        {
            if (Notify.IsWithin(StartTime, EndTime)) { OutNotifies.Add(Notify); }
        }
    }
    else if (StartTime > EndTime)  // 역방향으로 진행하는 경우
    {
        for (auto It = Notifies.rbegin(); It != Notifies.rend(); ++It)
        {
            if (It->IsWithin(EndTime, StartTime)) { OutNotifies.Add(*It); }
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
