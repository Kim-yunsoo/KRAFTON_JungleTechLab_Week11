#pragma once
#include "AnimSequenceBase.h"

class UAnimSequence : public UAnimSequenceBase
{
public:
    DECLARE_CLASS(UAnimSequence, UAnimSequenceBase);

    UAnimSequence() {}
    virtual ~UAnimSequence()
    {
    }

};

