#pragma once
#include "Object.h"
#include "UInputComponent.generated.h"

UCLASS(DisplayName = "인풋컴포넌트", Description = "사용자의 제어를 입력받는 오브젝트입니다.")
class UInputComponent : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

	UInputComponent() = default;

	// C++에서 사용 - 멤버 함수 포인터로 바인딩
	template<class UserClass>
	void BindAxis(const FString& AxisName, int KeyCode, float Scale, UserClass* Object, void(UserClass::*Func)(float))
	{
		// 중복 제거: 같은 KeyCode + 같은 Object 조합이면 기존 바인딩 삭제
		AxisBindings.erase(std::remove_if(AxisBindings.begin(), AxisBindings.end(),
			[&](const FAxisBinding& B) {return B.KeyCode == KeyCode && B.Object == Object;}),
			AxisBindings.end()
		);

		FAxisBinding Binding;
		Binding.AxisName = AxisName;
		Binding.KeyCode = KeyCode;
		Binding.Scale = Scale;
		Binding.Object = Object;
		Binding.FunctionName = "";  // C++에서는 함수 이름 불필요

		// 타입 소거: 멤버 함수 포인터를 void*로 저장
		Binding.FunctionPtr = *reinterpret_cast<void**>(&Func);

		// 호출 래퍼: 타입을 복원해서 실제 함수 호출
		Binding.ExecuteAxis = [](UObject* Obj, void* FuncPtr, float Value) {
			auto TypedFunc = *reinterpret_cast<void(UserClass::**)(float)>(&FuncPtr);
			(static_cast<UserClass*>(Obj)->*TypedFunc)(Value);
		};

		AxisBindings.Add(Binding);
	}

	template<class UserClass>
	void BindAction(const FString& ActionName, int KeyCode, UserClass* Object, void(UserClass::* Func)())
	{
		// 중복 제거: 같은 KeyCode + 같은 Object 조합이면 기존 바인딩 삭제
		ActionBindings.erase(std::remove_if(ActionBindings.begin(), ActionBindings.end(),
			[&](const FActionBinding& B) {return B.KeyCode == KeyCode && B.Object == Object;}),
			ActionBindings.end()
		);

		FActionBinding Binding;
		Binding.ActionName = ActionName;
		Binding.KeyCode = KeyCode;
		Binding.Object = Object;
		Binding.FunctionName = "";  // C++에서는 함수 이름 불필요

		// 타입 소거
		Binding.FunctionPtr = *reinterpret_cast<void**>(&Func);

		// 호출 래퍼
		Binding.ExecuteAction = [](UObject* Obj, void* FuncPtr) {
			auto TypedFunc = *reinterpret_cast<void(UserClass::**)()>(&FuncPtr);
			(static_cast<UserClass*>(Obj)->*TypedFunc)();
		};

		ActionBindings.Add(Binding);
	}

	// 루아 스크립트 연결용 래핑 함수
	void BindAxisByName(const FString& AxisName, int KeyCode, float Scale, UObject* Object, const FString& FunctionName)
	{
		BindAxisInternal(AxisName, KeyCode, Scale, Object, FunctionName);
	}

	// 루아 스크립트 연결용 래핑 함수
	void BindActionByName(const FString& ActionName, int KeyCode, UObject* Object, const FString& FunctionName)
	{
		BindActionInternal(ActionName, KeyCode, Object, FunctionName);
	}

	void ProcessInput();

protected:
	virtual ~UInputComponent() override = default;

private:
	// 루아 스크립트의 BindAxisByName 호출을 처리하는 함수
	void BindAxisInternal(const FString& AxisName, int KeyCode, float Scale, UObject* Object, const FString& FuncName);

	// 루아 스크립트의 BindActionByName 호출을 처리하는 함수
	void BindActionInternal(const FString& ActionName, int KeyCode, UObject* Object, const FString& FuncName);

	// 키가 눌려있는 기간, 매 프레임 처리를 위한 구조체
	struct FAxisBinding
	{
		FString AxisName;
		int KeyCode;
		float Scale;
		UObject* Object;										// 콜백 호출할 객체
		FString FunctionName;									// 루아용
		void* FunctionPtr = nullptr;							// C++용 
		void (*ExecuteAxis)(UObject*, void*, float) = nullptr;  // 호출 래퍼
	};

	// 키가 눌린 순간, 1회성 처리를 위한 구조체
	struct FActionBinding
	{
		FString ActionName;
		int KeyCode;
		UObject* Object;								   // 콜백 호출할 객체
		FString FunctionName;							   // 루아용
		void* FunctionPtr = nullptr;					   // C++용
		void (*ExecuteAction)(UObject*, void*) = nullptr;  // 호출 래퍼
	};

	TArray<FAxisBinding> AxisBindings;
	TArray<FActionBinding> ActionBindings;
};

