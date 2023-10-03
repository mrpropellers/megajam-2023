#pragma once

#include "CoreMinimal.h"
#include "RepFloatingMovement.h"
#include "GameFramework/Pawn.h"
#include "SubmarinePawn.generated.h"

class USubmarineWeapon;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartedDash);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCellPickedUp);

UCLASS()
class ANTIQUATEDFUTURE_API ASubmarinePawn : public APawn
{
	GENERATED_BODY()

// ------ MOVEMENT REPLICATION CODE --------
protected:
	const float ExtrapolationLimit = 0.1f;
	bool bHasWarnedAuthority;
	bool bWeaponsAreInitialized;
	bool bIsChargingJuggernautDash;
	float LastTimestampApplied;

	FString NetDebugName;
	
	// Not a UENUM so need custom function
	FString ToString(ENetMode NetMode) const;
	float Now() const;

	TArray<USubmarineWeapon*> Weapons;
	
	TWeakObjectPtr<AGameStateBase> GameState;
	void CalculateAndSendUpdates(float DeltaTime);
	void InitializeWeapons();
	void ApplyLastUpdate();

public:
	ASubmarinePawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Unreliable)
	void ServerSetTransform(const float Timestamp,
		const FVector_NetQuantize Position,
		const FQuat Rotation,
		const FVector_NetQuantize NewVelocity
		);

	bool IsLocalControl() const;
	bool IsAuthority() const;
	
	// UPROPERTY(Replicated)
	// bool bHasReceivedMovement;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsDashing;
	
	UPROPERTY(ReplicatedUsing = OnRep_Move)
	FRepFloatingMovement ServerMovement;
	UFUNCTION()
	void OnRep_Move();
	

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void Move(const FInputActionValue& ActionValue);
	void Rotate(const FInputActionValue& ActionValue);
	void Dash(const FInputActionValue& ActionValue);
	void JuggernautDash(const FInputActionValue& ActionValue);
	void DoJuggernautDash();
	void EndDash();

	
	UPROPERTY(EditAnywhere)
	class USphereComponent* Sphere;
	UPROPERTY(EditAnywhere)
	class USpringArmComponent* SpringArm;
	UPROPERTY(EditAnywhere)
	class UCameraComponent* Camera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UFloatingPawnMovement* Movement;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Juggernaut)
	bool bIsJuggernaut;
	UPROPERTY(EditAnywhere)
	float JuggernautDashCooldown;
	UPROPERTY(EditAnywhere)
	float DashCooldown;
	UPROPERTY(EditAnywhere)
	float MoveSensitivity;
	UPROPERTY(EditAnywhere)
	float RotateSensitivity;
	UPROPERTY(EditAnywhere)
	float CorrectiveSpeed;
	UPROPERTY(VisibleAnywhere)
	float RollCooldown;;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DashSpeed = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float JuggernautDashSpeed = 6000.0f;
	UPROPERTY(EditAnywhere)
	float DashSlowdown = 4000.0f;
	UPROPERTY(EditAnywhere)
	float DashDuration = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float JuggernautDashChargeDuration = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float JuggernautDashDuration;
	UPROPERTY(BlueprintReadOnly)
	float TimeJuggernautDashChargingStarted;

	UPROPERTY(BlueprintAssignable)
	FStartedDash DashStarted;
	UPROPERTY(BlueprintAssignable)
	FCellPickedUp CellPickedUp;

	UFUNCTION()
	void OnRep_Juggernaut();

protected:
	float CurrentDashCooldown;
	float TimeLastRolled;
	FVector LastKnownStrafeInput;
	float DefaultMaxSpeed;
	float TimeLastDashFinished;
	FTimerHandle DashEndTimerHandle;
	float DefaultDeceleration;
	float DefaultAcceleration;

	bool CanDash();
	bool IsRolling();
};
