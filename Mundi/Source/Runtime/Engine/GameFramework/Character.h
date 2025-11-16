#pragma once
#include "Pawn.h"
#include "ACharacter.generated.h"

UCLASS(DisplayName = "캐릭터", Description = "제어가 가능한 인간형 액터입니다.")
class ACharacter : public APawn
{
public:
	GENERATED_REFLECTION_BODY()

	ACharacter();

protected:
	virtual ~ACharacter() override;

private:

};

