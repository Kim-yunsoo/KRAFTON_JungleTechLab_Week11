#pragma once
#include "Object.h"

class UAnimAsset : public UObject
{
public:
    DECLARE_CLASS(UAnimAsset, UObject);

    UAnimAsset() = default;
    virtual ~UAnimAsset() override = default;

    // 모든 애니메이션(시퀀스, 블렌드스페이스 ..)은 길이를 보유
    virtual float GetPlayLength() const = 0;
};

