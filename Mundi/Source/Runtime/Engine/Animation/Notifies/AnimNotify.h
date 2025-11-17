#pragma once
#include "Source/Runtime/Core/Object/Object.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;
struct FAnimNotifyEvent;

class UAnimNotify : public UObject
{
    DECLARE_CLASS(UAnimNotify, UObject)
public:
    UAnimNotify() = default;
    virtual ~UAnimNotify() override = default;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEvent& Event) {}
};

