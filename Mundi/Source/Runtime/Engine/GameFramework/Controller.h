#pragma once
#include "Actor.h"
#include "AController.generated.h"

UCLASS(DisplayName = "컨트롤러", Description = "액터의 제어를 명령하는 액터입니다.")
class AController : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AController();

protected:
	virtual ~AController() override;

private:

};

