#include "pch.h"
#include "JamesCharacter.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include "InputComponent.h"
#include "InputManager.h"
#include "AnimSequence.h"
#include <windows.h>

AJamesCharacter::AJamesCharacter()
{
	// James 스켈레탈 메시 설정
	if (Mesh)
	{
		Mesh->SetSkeletalMesh("Data/James/James.fbx");
		// TODO: 메시 위치 조정 (캡슐 중심에서 아래로)
	}

	// 캡슐 크기 설정
	if (CapsuleComponent)
	{
		// TODO: James 크기에 맞게 조정
	}
}

void AJamesCharacter::BeginPlay()
{
	Super::BeginPlay();

	// TODO: 애니메이션 작동을 안함! 호민이형 해줘! (PYB)
	IdleAnimation = RESOURCE.Get<UAnimSequence>("Data/Animations/Breathing Idle.fbx");
	WalkAnimation = RESOURCE.Get<UAnimSequence>("Data/Animations/Standard Walk.fbx");
	RunAnimation = RESOURCE.Get<UAnimSequence>("Data/Animations/Standard Run.fbx");

	// 기본 Idle 애니메이션 재생
	if (Mesh && IdleAnimation)
	{
		Mesh->PlayAnimation(IdleAnimation->GetFilePath(), true);
		CurrentAnimation = IdleAnimation;
	}
}

void AJamesCharacter::SetupPlayerInputComponent()
{
	Super::SetupPlayerInputComponent();

	InputComponent->BindAction("StartRun", VK_SHIFT, this, &AJamesCharacter::StartRunning);
}

void AJamesCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UInputManager& Input = UInputManager::GetInstance();
	if (Input.IsKeyReleased(VK_SHIFT))
	{
		StopRunning();
	}

	if (!Input.IsKeyDown(VK_UP) && !Input.IsKeyDown(VK_DOWN))
	{
		CurrentVelocity.X = 0.0f;
	}
	if (!Input.IsKeyDown(VK_LEFT) && !Input.IsKeyDown(VK_RIGHT))
	{
		CurrentVelocity.Y = 0.0f;
	}

	// Pick animation based on current speed
	float Speed = CurrentVelocity.Size();

	if (Mesh)
	{
		UAnimSequence* DesiredAnimation = nullptr;

		if (Speed > 0.1f)  // Small epsilon to check movement
		{
			// Moving
			if (bIsRunning && RunAnimation)
			{
				DesiredAnimation = RunAnimation;
			}
			else if (WalkAnimation)
			{
				DesiredAnimation = WalkAnimation;
			}
		}
		else
		{
			// Idle
			if (IdleAnimation)
			{
				DesiredAnimation = IdleAnimation;
			}
		}

		// Only switch when animation changed to avoid restart
		if (DesiredAnimation && DesiredAnimation != CurrentAnimation)
		{
			Mesh->PlayAnimation(DesiredAnimation->GetFilePath(), true);
			CurrentAnimation = DesiredAnimation;
		}
	}

	// Update location when moving
	if (Speed > 0.01f)
	{
		FVector NewLocation = GetActorLocation() + CurrentVelocity * DeltaTime;
		SetActorLocation(NewLocation);
	}
}

void AJamesCharacter::MoveForward(float Value)
{
	FVector Forward = GetActorForward();
	float Speed = bIsRunning ? RunSpeed : WalkSpeed;
	CurrentVelocity.X = Forward.X * Value * Speed;
}

void AJamesCharacter::MoveRight(float Value)
{
	FVector Right = GetActorRight();
	float Speed = bIsRunning ? RunSpeed : WalkSpeed;
	CurrentVelocity.Y = Right.Y * Value * Speed;
}
