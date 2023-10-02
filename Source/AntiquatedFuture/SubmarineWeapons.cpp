// Fill out your copyright notice in the Description page of Project Settings.


#include "SubmarineWeapons.h"
#include "SubmarineProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
USubmarineWeapon::USubmarineWeapon()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	UActorComponent::SetComponentTickEnabled(true);

	BaseFireRate = 10.f;
	DummyProjectileLifeSpan = 0.2f;
}

void USubmarineWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(USubmarineWeapon, bIsShooting, COND_SkipOwner);
}

void USubmarineWeapon::OnRep_IsShooting()
{
	if (Instigator->IsLocallyControlled() || GetOwnerRole() != ROLE_SimulatedProxy)
	{
		UE_LOG(LogTemp, Error, TEXT("OnRep notify called somewhere besides Simulated Proxy! This shouldn't happen!"))
	}
	
	if (bIsShooting)
	{
		StartedShooting.Broadcast();
		TimeLastFired = Now();
		ShotFired.Broadcast();
	}
	else
	{
		StoppedShooting.Broadcast();
	}
}


// Called when the game starts
void USubmarineWeapon::BeginPlay()
{
	Super::BeginPlay();
	// IMPORTANT: We don't support modifying BaseFireRate during play
	PeriodBetweenShots = 1.f / BaseFireRate;
	bWasShootingLastTick = false;
	// Initialize events to an arbitrary point in the past
	TimeLastStoppedShooting = Now() - PeriodBetweenShots;
	TimeLastFired = TimeLastStoppedShooting - PeriodBetweenShots;
	NewProjectiles = TArray<TObjectPtr<ASubmarineProjectile>>();
	const auto ProjectileDefaultObject = Projectile->GetDefaultObject();
	TArray<UObject*> ProjectileComponents;
	ProjectileDefaultObject->GetDefaultSubobjects(ProjectileComponents);
	InitialProjectileSpeed = -1.f;
	for (int i = 0; i < ProjectileComponents.Num(); i++)
	{
		const auto MovementComponent = Cast<UProjectileMovementComponent>(ProjectileComponents[i]);
		if (MovementComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("Caching initial projectile speed: %f"), MovementComponent->InitialSpeed);
			InitialProjectileSpeed = MovementComponent->InitialSpeed;
			break;
		}
	}
	if (InitialProjectileSpeed < 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to cache projectile speed. This could cause problems!"));
	}

	// const auto RoleName = UEnum::GetValueAsString(GetOwnerRole());
	// UE_LOG(LogTemp, Warning, TEXT("%s starting with NET Role: %s"),
	// 	*GetOwner()->GetName(), *RoleName);
}

void USubmarineWeapon::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto CurrentTime = Now();
	// UE_LOG(LogTemp, Log, TEXT("Tick: %f"), CurrentTime)
	if (bIsShooting && CanShoot(CurrentTime))
	{
		// If this is the first shot since player started holding Shoot button, we're Starting
		if (Instigator->IsLocallyControlled())
		{
			if (TimeLastStoppedShooting > TimeLastFired)
			{
				StartShootingLocalOnly(CurrentTime);
			}
			else
			{
				InterpolateAndShoot(CurrentTime);
			}
		}
		// Otherwise we're just continuing to shoot
		else if (GetOwnerRole() == ROLE_Authority)
		{
			for (int i = 0; i < NewProjectiles.Num(); i++)
			{
				if (NewProjectiles[i])
				{
					NewProjectiles[i]->TearOff();
				}
			}
			NewProjectiles.Empty();
			InterpolateAndShoot(CurrentTime);
		}
		else if (GetOwnerRole() == ROLE_SimulatedProxy)
		{
			TimeLastFired = TimeLastFired + PeriodBetweenShots;
			ShotFired.Broadcast();
		}
	}
}


void USubmarineWeapon::BindToPlayer(
	USceneComponent* PlayerLook)
{
	const auto ActionName = ShootAction->StaticClass()->GetName();
	UE_LOG(LogTemp, Log, TEXT("Binding %s..."), *ActionName);
	PlayerLookComponent = PlayerLook;
}


// void USubmarineWeapon::AllClientsShoot_Implementation(const float TimeStamp, const FVector_NetQuantize10 Position,
// 	const FQuat Rotation)
// {
// }

bool USubmarineWeapon::CanShoot(const float TimeStamp)
{
	return TimeStamp - TimeLastFired > PeriodBetweenShots;
}

float USubmarineWeapon::Now() const
{
	return GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
}

void USubmarineWeapon::HandleShootAction(const FInputActionValue& ActionValue)
{
	const auto bIsShootAction = ActionValue.Get<bool>();
	UE_LOG(LogTemp, Log, TEXT("Shoot button pressed."))
	const auto TimeStamp = Now();
	if (bIsShootAction)
	{
	    if (CanShoot(TimeStamp))
	    {
            if (bShootOutOfPhase)
            {
                // Force this weapon to shoot halfway through fire cooldown instead of right away
                TimeLastStoppedShooting = Now();
                TimeLastFired = TimeLastStoppedShooting - PeriodBetweenShots * 0.5f;
                bIsShooting = true;
            }
            else
            {
	            StartShootingLocalOnly(TimeStamp);
            }
	    }
		else
		{
			bIsShooting = true;
		}
	}
	else if (!bIsShootAction)
	{
		StopShootingLocalOnly(TimeStamp);
	}
}

void USubmarineWeapon::StopShootingLocalOnly(const float TimeStamp)
{
	bIsShooting = false;
	TimeLastStoppedShooting = TimeStamp;
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerStopShooting(TimeStamp);
	}
}

void USubmarineWeapon::SetInstigator(APawn* OwningPawn)
{
	if (OwningPawn)
	{
		Instigator = OwningPawn;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("No Instigator to assign!"))
	}
}


void USubmarineWeapon::StartShootingLocalOnly(const float TimeStamp)
{
	bIsShooting = true;
	// This shouldn't happen...
	if (!Instigator->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invoked StartShooting from somewhere other than Owner or Server"))
		return;
	}
	// True bullets will be spawned by the Server and replicated
	// This just initializes our tracking of the simulated bullets without actually spawning anything
	ShootFromCurrentTransform(TimeStamp);

	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerStartShooting(TimeStamp, LastFiredPosition, LastFiredOrientation, LastFiredVelocity);
	}
}

void USubmarineWeapon::ServerStartShooting_Implementation(const float TimeStamp,
	const FVector_NetQuantize CurrentPosition, const FQuat CurrentRotation, const FVector_NetQuantize10 CurrentVelocity)
{
	if (bIsShooting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server told to Start shooting multiple times in a row. How?!"))
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Server starting to shoot."))
	}
	bIsShooting = true;

	ShootProjectile(TimeStamp, CurrentVelocity, CurrentPosition, CurrentRotation);
	//MulticastStartShooting(TimeStamp, CurrentPosition, CurrentRotation, CurrentVelocity);
}

void USubmarineWeapon::ServerStopShooting_Implementation(const float TimeStamp)
{
	if (bIsShooting)
	{
		UE_LOG(LogTemp, Log, TEXT("Server stopping shooting."))
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Server told to Stop shooting multiple times in a row?!"));
	}
	bIsShooting = false;
	TimeLastStoppedShooting = TimeStamp;
	//MulticastStopShooting(TimeStamp);
}

// void USubmarineWeapon::ServerShoot_Implementation(const float TimeStamp, const FVector_NetQuantize10 Position,
// 	const FQuat Rotation)
// {
// }

void USubmarineWeapon::InterpolateAndShoot(const float CurrentTime)
{
	float TimeOvershoot = CurrentTime - TimeLastFired;
	if (TimeOvershoot < PeriodBetweenShots - FLT_EPSILON)
	{
		UE_LOG(LogTemp, Error, TEXT("Attempting to shoot before cooldown elapsed..."))
		return;
	}
	if (TimeOvershoot > 2 * PeriodBetweenShots && TimeLastStoppedShooting < TimeLastFired)
	{
		if (TimeLastStoppedShooting < TimeLastFired)
		{
			UE_LOG(LogTemp, Log, TEXT("Too much Tick time is elapsing between shots. Making up the difference"))
			InterpolateAndShoot(CurrentTime - PeriodBetweenShots);
			// The recursive call will have updated TimeLastFired, meaning we should calculate the new overshoot
			TimeOvershoot = CurrentTime - TimeLastFired;
			if (TimeOvershoot > 2 * PeriodBetweenShots)
			{
				UE_LOG(LogTemp, Error, TEXT("Recursive call should have taken care of that..."))
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("I'm pretty sure this can't happen. Resetting shot history."))
			ShootFromCurrentTransform(CurrentTime);
		}
	}

	const auto Transform = GetComponentTransform();
	const auto t = PeriodBetweenShots / TimeOvershoot;
	const FVector_NetQuantize ShotLocation = FMath::Lerp(LastFiredPosition, Transform.GetLocation(), t);
    const FQuat ShotOrientation = FMath::Lerp(LastFiredOrientation, Transform.GetRotation(), t);
	const FVector_NetQuantize10 ShotVelocity = FMath::Lerp(LastFiredVelocity, GetOwner()->GetVelocity(), t);
	ShootProjectile(TimeLastFired + PeriodBetweenShots, ShotVelocity, ShotLocation, ShotOrientation);
}

ASubmarineProjectile* USubmarineWeapon::ShootFromCurrentTransform(const float TimeStamp)
{
	const FVector_NetQuantize SpawnLocation = GetComponentTransform().GetLocation();
	const FQuat SpawnRotation = PlayerLookComponent->GetComponentQuat();
	return ShootProjectile(TimeStamp, GetOwner()->GetVelocity(), SpawnLocation, SpawnRotation);
}

ASubmarineProjectile* USubmarineWeapon::ShootProjectile(
	const float TimeStamp,
	const FVector_NetQuantize10& InheritedVelocity, 
	const FVector_NetQuantize& Position,
	const FQuat& Rotation)
{
	const auto CurrentTime = Now();
	float DeltaTime = CurrentTime - TimeStamp;
	if (DeltaTime < -FLT_EPSILON)
	{
		UE_LOG(LogTemp, Warning, TEXT(
			"Attempting to spawn projectile %f seconds before it was fired. Setting to 0."), DeltaTime);
		DeltaTime = 0;
	}

	ASubmarineProjectile* SubmarineProjectile = nullptr;
	if (GetOwnerRole() == ROLE_Authority)
	{
		SubmarineProjectile = SpawnProjectile(DeltaTime, InheritedVelocity, Position, Rotation);
		if (SubmarineProjectile)
		{
			// TearOff these projectiles after they've had a chance to Tick since the client-side sim is visual only
			// Add them to this list to be torn off next frame
			NewProjectiles.Add(SubmarineProjectile);
		}
	}
	else if (Instigator->IsLocallyControlled())
	{
		SubmarineProjectile = SpawnProjectile(DeltaTime, InheritedVelocity, Position, Rotation);
		if (SubmarineProjectile)
		{
			SubmarineProjectile->SetLifeSpan(DummyProjectileLifeSpan);
		}
	}
	
	TimeLastFired = TimeStamp;
	LastFiredVelocity = InheritedVelocity;
	LastFiredPosition = Position;
	LastFiredOrientation = Rotation;
	ShotFired.Broadcast();
	
	return SubmarineProjectile;
}

ASubmarineProjectile* USubmarineWeapon::SpawnProjectile(
	const float DeltaTime, const FVector_NetQuantize10& InheritedVelocity, const FVector_NetQuantize& Position, const FQuat& Rotation)
{
	if (Instigator == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("No Instigator set in SubmarineWeapons - did it fail to Replicate?"))
	}
	const auto ProjectileForwardVelocity = Rotation.GetForwardVector() * InitialProjectileSpeed;
	const auto Displacement = DeltaTime * ProjectileForwardVelocity;
	const FVector CurrentLocation = Position + Displacement;
	const FRotator Rotator = Rotation.Rotator();
	FActorSpawnParameters ActorSpawnParameters = FActorSpawnParameters();
	//ActorSpawnParameters.Owner = GetOwner();
	ActorSpawnParameters.Instigator = Instigator;
	const auto ProjectileDefaultObject = Projectile->GetDefaultObject();
	const auto ProjectileClass = ProjectileDefaultObject->GetClass();
	ASubmarineProjectile* SubmarineProjectile = nullptr;
	if (const auto ProjectileActor = GetWorld()->SpawnActor(
		ProjectileClass, &CurrentLocation, &Rotator, ActorSpawnParameters))
	{
		SubmarineProjectile = Cast<ASubmarineProjectile>(ProjectileActor);
		if (SubmarineProjectile)
		{
			if (const auto ProjectileMovement = SubmarineProjectile->GetComponentByClass<UProjectileMovementComponent>())
			{
				ProjectileMovement->Velocity += InheritedVelocity;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get ProjectileMovementComponent!"))
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to cast to SubmarineProjectile!"))
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn projectile!"))
	}

	return SubmarineProjectile;
}


// void USubmarineWeapon::MulticastStopShooting_Implementation(const float TimeStamp)
// {
// 	if (GetOwnerRole() == ROLE_AutonomousProxy)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Ignoring StopShooting because we should already be shooting."));
// 		return;
// 	}
// 	if (bIsShooting)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Client stopping shooting."))
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Client told to Stop shooting multiple times in a row?!"));
// 	}
// 	bIsShooting = false;
// 	TimeLastStoppedShooting = TimeStamp;
// }
// 
// void USubmarineWeapon::MulticastStartShooting_Implementation(const float TimeStamp,
// 	const FVector_NetQuantize CurrentPosition, const FQuat CurrentRotation, const FVector_NetQuantize10 CurrentVelocity)
// {
// 	if (GetOwnerRole() == ROLE_AutonomousProxy)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Ignoring StartShooting because we should already be shooting."));
// 		return;
// 	}
// 
// 	if (bIsShooting)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Client told to Start shooting multiple times in a row. How?!"))
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Client starting to shoot."))
// 	}
// 	bIsShooting = true;
// 
// 	ShootProjectile(TimeStamp, CurrentVelocity, CurrentPosition, CurrentRotation);
// }

