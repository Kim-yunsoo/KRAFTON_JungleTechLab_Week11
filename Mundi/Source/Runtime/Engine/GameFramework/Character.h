#pragma once
#include "Pawn.h"
#include "ACharacter.generated.h"

class UMovementComponent;
class USkeletalMeshComponent;
class UCapsuleComponent;

UCLASS(DisplayName = "캐릭터", Description = "제어가 가능한 인간형 액터입니다.")
class ACharacter : public APawn
{
public:
	GENERATED_REFLECTION_BODY()

	ACharacter();

	// 입력 설정 오버라이드
	virtual void SetupPlayerInputComponent() override;

	// 이동 함수들 - InputComponent에서 호출됨
	virtual void MoveForward(float Value);
	virtual void MoveRight(float Value);

	// 컴포넌트 접근자
	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

	// 복사
	void DuplicateSubObjects() override;

	// 직렬화
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	virtual ~ACharacter() override = default;

	// 이동 속성
	float BaseTurnRate = 45.0f;		// 초당 도 단위

	// 캐릭터 컴포넌트들
	USkeletalMeshComponent* Mesh = nullptr;
	UCapsuleComponent* CapsuleComponent = nullptr;
};

