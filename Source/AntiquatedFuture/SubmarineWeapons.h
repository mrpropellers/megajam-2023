// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Components/ActorComponent.h"
#include "SubmarineWeapons.generated.h"

class UInputAction;
class ASubmarineProjectile;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartedShooting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShotFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStoppedShooting);

UCLASS(ClassGroup=(Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class ANTIQUATEDFUTURE_API USubmarineWeapon : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USubmarineWeapon();

protected:
	// Called when the game starts

	bool bWasShootingLastTick;
	bool bIsDisabledBecauseJuggernaut;
	float PeriodBetweenShots;

	FVector_NetQuantize LastFiredPosition;
	FQuat LastFiredOrientation;
	FVector_NetQuantize10 LastFiredVelocity;
	float TimeLastFired;
	float TimeLastStoppedShooting;
	float InitialProjectileSpeed;
	
	TWeakObjectPtr<USceneComponent> PlayerLookComponent;
	// TODO? Optimize FQuat by encoding into normalized FVector of Euler angles?
	// UFUNCTION(NetMulticast, Reliable)
	// void AllClientsStartShooting(
	// 	const float TimeStamp,
	// 	const FVector_NetQuantize CurrentPosition,
	// 	const FQuat CurrentRotation,
	// 	const FVector_NetQuantize10 CurrentVelocity);
	void InterpolateAndShoot(const float TimeStamp);
	ASubmarineProjectile* ShootFromCurrentTransform(const float TimeStamp);
	ASubmarineProjectile* ShootProjectile(
		const float TimeStamp,
		const FVector_NetQuantize10& InheritedVelocity,
		const FVector_NetQuantize& Position,
		const FQuat& Rotation);
	ASubmarineProjectile* SpawnProjectile(
		const float DeltaTime,
		const FVector_NetQuantize10& InheritedVelocity,
		const FVector_NetQuantize& Position,
		const FQuat& Rotation);

	UPROPERTY()
	TArray<TObjectPtr<ASubmarineProjectile>> NewProjectiles; 
	UPROPERTY()
	TObjectPtr<APawn> Instigator;

	// How long the Autonomous Proxy's (visual only) projectiles live for
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DummyProjectileLifeSpan;

	UFUNCTION(Server, Reliable)
	void ServerStartShooting(
		const float TimeStamp,
		const FVector_NetQuantize CurrentPosition,
		const FQuat CurrentRotation,
		const FVector_NetQuantize10 CurrentVelocity);
	UFUNCTION(Server, Reliable)
	void ServerStopShooting(const float TimeStamp);
	// UFUNCTION(NetMulticast, Reliable)
	// void MulticastStartShooting(
	// 	const float TimeStamp,
	// 	const FVector_NetQuantize CurrentPosition,
	// 	const FQuat CurrentRotation,
	// 	const FVector_NetQuantize10 CurrentVelocity);
	// UFUNCTION(NetMulticast, Reliable)
	// void MulticastStopShooting(const float TimeStamp);

public:
	// Called every frame
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	// 	FActorComponentTickFunction* ThisTickFunction) override;
	float Now() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_IsShooting)
	bool bIsShooting;
	UFUNCTION()
	void OnRep_IsShooting();

	void HandleShootAction(const FInputActionValue& ActionValue);
	virtual bool CanShoot(const float TimeStamp);
	void StartShootingLocalOnly(const float TimeStamp);
	void StopShootingLocalOnly(const float TimeStamp);
	void SetInstigator(APawn* OwningPawn);
	
	UPROPERTY(EditAnywhere)
	bool bShootOutOfPhase;
	
	// Fire rate in bullets/second
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	float BaseFireRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<class ASubmarineProjectile> Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ShootAction;

	UPROPERTY(BlueprintAssignable)
	FStartedShooting StartedShooting;
	UPROPERTY(BlueprintAssignable)
	FShotFired ShotFired;
	UPROPERTY(BlueprintAssignable)
	FStoppedShooting StoppedShooting;

	UFUNCTION(BlueprintCallable)
	void OnCellPickedUp();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BindToPlayer(USceneComponent* PlayerLook);
};
