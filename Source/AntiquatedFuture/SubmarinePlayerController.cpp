#include "SubmarinePlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

ASubmarinePlayerController::ASubmarinePlayerController()
{
	PawnMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("Input Mappings"));

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("Move Action"));
	MoveAction->ValueType = EInputActionValueType::Axis3D;

	RotateAction = CreateDefaultSubobject<UInputAction>(TEXT("Rotate Action"));
	RotateAction->ValueType = EInputActionValueType::Axis3D;

	DashAction = CreateDefaultSubobject<UInputAction>(TEXT("Dash Action"));
	DashAction->ValueType = EInputActionValueType::Boolean;
}


static void MapKey(UInputMappingContext* InputMappingContext, UInputAction* InputAction, FKey Key,
	bool bNegate = false, bool bSwizzle = false, EInputAxisSwizzle SwizzleOrder = EInputAxisSwizzle::YXZ)
{
	FEnhancedActionKeyMapping& Mapping = InputMappingContext->MapKey(InputAction, Key);
	UObject* Outer = InputMappingContext->GetOuter();

	if (bNegate)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Outer);
		Mapping.Modifiers.Add(Negate);
	}

	if (bSwizzle)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Outer);
		Swizzle->Order = SwizzleOrder;
		Mapping.Modifiers.Add(Swizzle);
	}
}

void ASubmarinePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Keyboard setup
	// Strafe movement
	MapKey(PawnMappingContext, MoveAction, EKeys::W);
	MapKey(PawnMappingContext, MoveAction, EKeys::S, true);
	MapKey(PawnMappingContext, MoveAction, EKeys::A, true, true, EInputAxisSwizzle::YXZ);
	MapKey(PawnMappingContext, MoveAction, EKeys::D, false, true, EInputAxisSwizzle::YXZ);

	// Up and down movement
	MapKey(PawnMappingContext, MoveAction, EKeys::SpaceBar, false, true, EInputAxisSwizzle::ZYX);
	MapKey(PawnMappingContext, MoveAction, EKeys::LeftShift, true, true, EInputAxisSwizzle::ZYX);

	// Pitch and yaw rotation
	MapKey(PawnMappingContext, RotateAction, EKeys::MouseY);
	MapKey(PawnMappingContext, RotateAction, EKeys::MouseX, false, true, EInputAxisSwizzle::YXZ);

	// Roll rotation
	MapKey(PawnMappingContext, RotateAction, EKeys::Q, true, true, EInputAxisSwizzle::ZYX);
	MapKey(PawnMappingContext, RotateAction, EKeys::E, false, true, EInputAxisSwizzle::ZYX);

	// Gamepad setup
	// Strafe movement
	MapKey(PawnMappingContext, MoveAction, EKeys::Gamepad_LeftY);
	MapKey(PawnMappingContext, MoveAction, EKeys::Gamepad_LeftX, false, true, EInputAxisSwizzle::YXZ);

	// Up and down movement
	MapKey(PawnMappingContext, MoveAction, EKeys::Gamepad_LeftShoulder, false, true, EInputAxisSwizzle::ZYX);
	MapKey(PawnMappingContext, MoveAction, EKeys::Gamepad_LeftTrigger, true, true, EInputAxisSwizzle::ZYX);

	// Pitch and yaw rotation
	MapKey(PawnMappingContext, RotateAction, EKeys::Gamepad_RightY, true);
	MapKey(PawnMappingContext, RotateAction, EKeys::Gamepad_RightX, false, true, EInputAxisSwizzle::YXZ);

	// Roll rotation
	MapKey(PawnMappingContext, RotateAction, EKeys::Gamepad_FaceButton_Left, true, true, EInputAxisSwizzle::ZYX);
	MapKey(PawnMappingContext, RotateAction, EKeys::Gamepad_FaceButton_Bottom, false, true, EInputAxisSwizzle::ZYX);

	// Dash
	MapKey(PawnMappingContext, DashAction, EKeys::Gamepad_FaceButton_Right);
	MapKey(PawnMappingContext, DashAction, EKeys::RightMouseButton);

	MapKey(PawnMappingContext, ShootPrimaryAction, EKeys::Gamepad_RightTrigger);
	MapKey(PawnMappingContext, ShootPrimaryAction, EKeys::LeftMouseButton);
}
