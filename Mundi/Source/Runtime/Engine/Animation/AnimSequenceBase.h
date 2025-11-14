#pragma once
#include "AnimAsset.h"

class UAnimSequenceBase : public UAnimAsset
{
public:
    DECLARE_CLASS(UAnimSequenceBase, UAnimAsset);

    UAnimSequenceBase() = default;
    virtual ~UAnimSequenceBase() override = default;

    virtual float GetPlayLength() const override = 0;
};

