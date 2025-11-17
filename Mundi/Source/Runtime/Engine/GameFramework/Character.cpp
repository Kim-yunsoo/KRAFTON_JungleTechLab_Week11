#include "pch.h"
#include "Character.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include <windows.h>

ACharacter::ACharacter()
{
	// 기본 회전 속도 설정
	BaseTurnRate = 45.0f;

	// 캡슐 컴포넌트 생성 (충돌) - RootComponent로 설정
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	RootComponent = CapsuleComponent;

	// 스켈레탈 메시 컴포넌트 생성 (시각적 표현)
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("CharacterMesh");
	// Mesh를 CapsuleComponent에 Attach - 이렇게 해야 Character 이동 시 함께 움직임
	Mesh->SetupAttachment(CapsuleComponent);
}

void ACharacter::SetupPlayerInputComponent()
{
	Super::SetupPlayerInputComponent();

	// InputComponent가 없으면 리턴
	if (!InputComponent) { return; }

	// 이동 입력 바인딩 (WASD)
	InputComponent->BindAxis("MoveForward", 'W', 1.0f, this, &ACharacter::MoveForward);
	InputComponent->BindAxis("MoveForward", 'S', -1.0f, this, &ACharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", 'D', 1.0f, this, &ACharacter::MoveRight);
	InputComponent->BindAxis("MoveRight", 'A', -1.0f, this, &ACharacter::MoveRight);

	// 회전 입력 바인딩 (화살표 키)
	InputComponent->BindAxis("Turn", VK_RIGHT, 1.0f, this, &ACharacter::Turn);
	InputComponent->BindAxis("Turn", VK_LEFT, -1.0f, this, &ACharacter::Turn);
}

void ACharacter::MoveForward(float Value)
{
	if (Value != 0.0f && MovementComponent)
	{
		// TODO: Transform의 Forward 방향으로 이동 입력 추가
		// 현재는 MovementComponent가 있으면 단순히 속도 설정
		FVector CurrentVelocity = MovementComponent->GetVelocity();
		// Forward 방향 (임시로 Y축 사용)
		CurrentVelocity.Y = Value * 100.0f;
		MovementComponent->SetVelocity(CurrentVelocity);
	}
}

void ACharacter::MoveRight(float Value)
{
	if (Value != 0.0f && MovementComponent)
	{
		// TODO: Transform의 Right 방향으로 이동 입력 추가
		// 현재는 MovementComponent가 있으면 단순히 속도 설정
		FVector CurrentVelocity = MovementComponent->GetVelocity();
		// Right 방향 (임시로 X축 사용)
		CurrentVelocity.X = Value * 100.0f;
		MovementComponent->SetVelocity(CurrentVelocity);
	}
}

void ACharacter::Turn(float Value)
{
	if (Value != 0.0f)
	{
		// TODO: Rotation 업데이트
		// 임시 구현: BaseTurnRate * Value * DeltaTime만큼 회전
		// DeltaTime은 Tick에서 받아와야 함
	}
}

void ACharacter::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// 복사 시 OwnedComponents에서 컴포넌트를 찾아 멤버 변수 업데이트
	for (UActorComponent* Component : OwnedComponents)
	{
		if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
		{
			Mesh = SkeletalMeshComp;
		}
		else if (UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(Component))
		{
			CapsuleComponent = CapsuleComp;
		}
	}
}

void ACharacter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// 로딩 시 컴포넌트 복원
		CapsuleComponent = Cast<UCapsuleComponent>(RootComponent);

		for (UActorComponent* Component : OwnedComponents)
		{
			if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(Component))
			{
				Mesh = SkeletalMeshComp;
				break;
			}
		}
	}
}
