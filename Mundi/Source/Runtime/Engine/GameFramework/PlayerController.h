#pragma once
#include "Controller.h"
#include "APlayerController.generated.h"

UCLASS(DisplayName = "플레이어 컨트롤러", Description = "플레이어의 제어를 명령하는 액터입니다.")
class APlayerController : public AController
{
public:
	GENERATED_REFLECTION_BODY()

	APlayerController();

protected:
	virtual ~APlayerController() override;

private:

};

