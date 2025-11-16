#pragma once
#include "Object.h"
#include "UInputComponent.generated.h"

UCLASS(DisplayName = "인풋컴포넌트", Description = "사용자의 제어를 입력받는 오브젝트입니다.")
class UInputComponent : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

	UInputComponent();

protected:
	virtual ~UInputComponent() override;

private:

};

