#pragma once
#include "Actor.h"
#include "APawn.generated.h"

UCLASS(DisplayName = "폰", Description = "제어가 가능한 액터입니다.")
class APawn : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	APawn();

protected:
	virtual ~APawn() override;

private:

};

