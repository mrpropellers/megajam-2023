#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerController.h"
#include "SubmarinePlayerController.generated.h"

class UInputAction;

UCLASS()
class ANTIQUATEDFUTURE_API ASubmarinePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASubmarinePlayerController();
	
	virtual void SetupInputComponent() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	class UInputMappingContext* PawnMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* RotateAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UInputAction> ShootPrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInputAction* DashAction;
};
